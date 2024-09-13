#include <queue>

#include <dlfcn.h>

#include <libgccjit.h>

#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "thunder/qualified_type.hpp"
#include "thunder/properties.hpp"
#include "thunder/gcc_jit_generator.hpp"

// Intrinsic implementations
// TODO: separate header file
extern "C" float clamp(float x, float low, float high)
{
	return std::min(high, std::max(low, x));
}

namespace jvl::thunder {

MODULE(gcc-jit);

#define LOCATION(context) gcc_jit_context_new_location(context, __FILE__, __LINE__, 0)

gcc_jit_type *generate_primitive_scalar_type(gcc_jit_context *const context, PrimitiveType item)
{
	switch (item) {
	case boolean:
		return gcc_jit_context_get_type(context, GCC_JIT_TYPE_BOOL);
	case i32:
		return gcc_jit_context_get_type(context, GCC_JIT_TYPE_INT32_T);
	case u32:
		return gcc_jit_context_get_type(context, GCC_JIT_TYPE_UINT32_T);
	case f32:
		return gcc_jit_context_get_type(context, GCC_JIT_TYPE_FLOAT);
	default:
		break;
	}

	JVL_ABORT("unsupported primitive type {} encountered in {}",
		tbl_primitive_types[item], __FUNCTION__);
}

gcc_jit_type *generate_primitive_vector_type(gcc_jit_context *context,
					const char *name,
					PrimitiveType item,
					size_t components,
					bool align)
{
	static constexpr const char *component_names[] = { "x", "y", "z", "w" };

	JVL_ASSERT(vector_component_count(item) == 0,
		"expected a primitive type for vector "
		"construction, got {} instead",
		tbl_primitive_types[item]);

	auto element = generate_primitive_scalar_type(context, item);

	std::vector <gcc_jit_field *> fields(components);
	for (size_t i = 0; i < components; i++) {
		fields[i] = gcc_jit_context_new_field(context,
			nullptr, element, component_names[i]);
	}

	if (align) {
		fmt::println("requires alignment, padding with an extra element");

		gcc_jit_field *extra = gcc_jit_context_new_field(context,
			LOCATION(context), element, "_align");

		fields.push_back(extra);
	}

	gcc_jit_struct *vector = gcc_jit_context_new_struct_type(context,
		LOCATION(context),
		name,
		fields.size(),
		fields.data());

	JVL_INFO("created struct for vector "
		"primitive type {}x{} (align={})",
		tbl_primitive_types[item], components,
		align);

	return gcc_jit_struct_as_type(vector);
}

gcc_jit_type *generate_primitive_type(gcc_jit_context *const context, PrimitiveType item)
{
	switch (item) {

	// Scalar types
	case boolean:
	case i32:
	case u32:
	case f32:
		return generate_primitive_scalar_type(context, item);

	// Integer vector types
	case ivec2:
		return generate_primitive_vector_type(context, "ivec2", i32, 2, false);
	case ivec3:
		return generate_primitive_vector_type(context, "ivec3", i32, 3, true);
	case ivec4:
		return generate_primitive_vector_type(context, "ivec4", i32, 4, false);

	// Unsigned integer vector types
	case uvec2:
		return generate_primitive_vector_type(context, "uvec2", u32, 2, false);
	case uvec3:
		return generate_primitive_vector_type(context, "uvec3", u32, 3, true);
	case uvec4:
		return generate_primitive_vector_type(context, "uvec4", u32, 4, false);

	// Floating-point vector types
	case vec2:
		return generate_primitive_vector_type(context, "vec2", f32, 2, false);
	case vec3:
		return generate_primitive_vector_type(context, "vec3", f32, 3, true);
	case vec4:
		return generate_primitive_vector_type(context, "vec4", f32, 4, false);

	default:
		break;
	}

	JVL_ABORT("unsupported primitive type {} encountered in {}",
		tbl_primitive_types[item], __FUNCTION__);
}

gcc_jit_object *generate_primitive(gcc_jit_context *const context, const Primitive &primitive)
{
	auto scalar_type = generate_primitive_scalar_type(context, primitive.type);

	switch (primitive.type) {

	case i32:
	{
		auto rv = gcc_jit_context_new_rvalue_from_int(context, scalar_type, primitive.idata);
		return gcc_jit_rvalue_as_object(rv);
	}

	case u32:
	{
		auto rv = gcc_jit_context_new_rvalue_from_int(context, scalar_type, primitive.udata);
		return gcc_jit_rvalue_as_object(rv);
	}

	case f32:
	{
		auto rv = gcc_jit_context_new_rvalue_from_double(context, scalar_type, primitive.fdata);
		return gcc_jit_rvalue_as_object(rv);
	}

	default:
		break;
	}

	JVL_ABORT("unsupported primitive: {}", primitive);
}

gcc_jit_object *generate_operation(gcc_jit_context *const context,
				   OperationCode code,
				   gcc_jit_type *value_type,
				   gcc_jit_rvalue *one,
				   gcc_jit_rvalue *two)
{
	static const wrapped::hash_table <OperationCode, gcc_jit_binary_op> table {
		{ addition,		GCC_JIT_BINARY_OP_PLUS		},
		{ subtraction,		GCC_JIT_BINARY_OP_MINUS		},
		{ multiplication,	GCC_JIT_BINARY_OP_MULT		},
		{ division,		GCC_JIT_BINARY_OP_DIVIDE	},
		{ bit_and,		GCC_JIT_BINARY_OP_BITWISE_AND	},
		{ bit_or,		GCC_JIT_BINARY_OP_BITWISE_OR	},
		{ bit_xor,		GCC_JIT_BINARY_OP_BITWISE_XOR	},
		{ bit_shift_left,	GCC_JIT_BINARY_OP_LSHIFT	},
		{ bit_shift_right,	GCC_JIT_BINARY_OP_RSHIFT	},
	};

	if (auto gcc_code = table.get(code)) {
		gcc_jit_rvalue *expr = gcc_jit_context_new_binary_op(context,
			nullptr, gcc_code.value(), value_type, one, two);

		JVL_ASSERT_PLAIN(expr);

		return gcc_jit_rvalue_as_object(expr);
	}

	JVL_ABORT("unsupported operation: {}", tbl_operation_code[code]);
}

struct intrinsic_lookup_info {
	IntrinsicOperation opn;
	std::vector <PrimitiveType> types;
	gcc_jit_type *returns;

	bool match(const std::vector <PrimitiveType> &other) const {
		if (types.size() != other.size())
			return false;

		for (size_t i = 0; i < types.size(); i++) {
			if (types[i] != other[i])
				return false;
		}

		return true;
	}
};

template <typename ... Args>
auto overload(const Args &... args)
{
	return std::vector <PrimitiveType> { args... };
}

gcc_jit_function *intrinsic_lookup(gcc_jit_context *const context, const intrinsic_lookup_info &info)
{
	switch (info.opn) {

	case clamp:
	{
		static const auto float_overload = overload(f32, f32, f32);

		if (info.match(float_overload)) {
			gcc_jit_type *float_type = gcc_jit_context_get_type(context, GCC_JIT_TYPE_FLOAT);
			
			gcc_jit_param *parameters[3];
			parameters[0] = gcc_jit_context_new_param(context, LOCATION(context), float_type, "x");
			parameters[1] = gcc_jit_context_new_param(context, LOCATION(context), float_type, "low");
			parameters[2] = gcc_jit_context_new_param(context, LOCATION(context), float_type, "high");

			return gcc_jit_context_new_function(context,
				LOCATION(context), GCC_JIT_FUNCTION_IMPORTED, float_type,
				"clamp", 3, parameters, 0);
		}
	}

	case sin:
	case cos:
	case tan:
	case asin:
	case acos:
	case atan:
	{
		static const auto float_overload = overload(f32);
			
		static const wrapped::hash_table <IntrinsicOperation, const char *const> float_names {
			{ sin, "sinf" },
			{ cos, "cosf" },
			{ tan, "tanf" },
			{ acos, "acosf" },
		};

		if (info.match(float_overload)) {
			JVL_ASSERT_PLAIN(float_names.contains(info.opn));
			return gcc_jit_context_get_builtin_function(context, float_names.at(info.opn));
		}
	}

	case pow:
	{
		static const auto float_overload = overload(f32, f32);

		if (info.match(float_overload))
			return gcc_jit_context_get_builtin_function(context, "powf");
	}

	// Intrinsics which could be supported if they had been lowered properly
	case dot:
		JVL_ABORT("intrinsic instruction ${} must be lowered", tbl_intrinsic_operation[info.opn]);

	default:
		break;
	}

	JVL_ABORT("unhandled case in intrinsic lookup: {}", tbl_intrinsic_operation[info.opn]);
}

namespace detail {

gcc_jit_function_generator_t::gcc_jit_function_generator_t(gcc_jit_context *const context_, const Buffer &buffer_)
		: Buffer(buffer_), context(context_) {}

// Generating types
gcc_jit_type *gcc_jit_function_generator_t::jitify_type(QualifiedType qt)
{
	auto original = qt;

	// libgccjit is sensitive to type addresses,
	// so we need to cache types we have already generated
	if (mapped_types.contains(original))
		return mapped_types[original];

	switch (qt.index()) {

	variant_case(QualifiedType, PlainDataType):
	{
		auto &pd = qt.as <PlainDataType> ();
		if (pd.is <PrimitiveType> ()) {
			auto item = pd.as <PrimitiveType> ();
			auto t = generate_primitive_type(context, item);
			return (mapped_types[qt] = t);
		}

		index_t concrete = pd.as <index_t> ();
		return jitify_type(types[concrete]);
	} break;

	variant_case(QualifiedType, StructFieldType):
	{
		fmt::println("struct field type:");
		std::vector <gcc_jit_field *> fields;

		index_t f = 0;
		while (!qt.is <NilType> ()) {
			fmt::println("  current {}", qt);

			gcc_jit_type *type = nullptr;
			if (qt.is <StructFieldType> ()) {
				auto &sft = qt.as <StructFieldType> ();
				type = jitify_type(sft.base());

				index_t next = qt.as <StructFieldType> ().next;
				qt = types[next];
			} else {
				type = jitify_type(qt);

				qt = QualifiedType::nil();
			}

			std::string name = fmt::format("f{}", f++);
			auto field = gcc_jit_context_new_field(context,
				LOCATION(context), type, name.c_str());

			fields.push_back(field);
		}

		std::string name = fmt::format("aggr{}", mapped_types.size());
		auto aggregate_struct = gcc_jit_context_new_struct_type(context,
			LOCATION(context), name.c_str(), fields.size(), fields.data());

		auto aggregate_type = gcc_jit_struct_as_type(aggregate_struct);
		mapped_types[original] = aggregate_type;
		return aggregate_type;
	}

	default:
		break;
	}

	JVL_ABORT("failed to JIT (gcc-jit) the following type: {}", original);
}

// Per-atom generator
template <atom_instruction T>
gcc_jit_object *generate(const T &atom, index_t i) {
	JVL_ABORT("failed to JIT (gcc-jit) compile atom: {} (@{})", atom, i);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Qualifier &, index_t)
{
	// Nothing to do here...
	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const TypeInformation &, index_t index)
{
	auto &qt = types[index];

	// All we need to do it generate the type so it is ready
	auto t = jitify_type(qt);
	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Primitive &primivite, index_t index)
{
	return generate_primitive(context, primivite);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Construct &constructor, index_t index)
{
	if (constructor.transient) {
		auto &atom = atoms[constructor.type];
		JVL_ASSERT(atom.is <Qualifier> (), "transient constructors must be typed to qualifiers");

		auto &qualifier = atom.as <Qualifier> ();
		if (qualifier.kind == parameter) {
			index_t loc = qualifier.numerical;
			auto rv = gcc_jit_param_as_rvalue(parameters[loc]);
			return gcc_jit_rvalue_as_object(rv);
		}

		JVL_ABORT("unhandled case for transient constructors: {}", qualifier);
	}

	JVL_ABORT("unfinished implementation for JIT-ing constructor: {}", constructor);
}

gcc_jit_object *gcc_jit_function_generator_t::load_field(index_t src, index_t index, bool lvalue)
{
	JVL_ASSERT(!lvalue, "lvalue loading is not implemented");

	// Find the original type
	QualifiedType original = types[src];
	// TODO: is_aggregate method()
	// TODO: double check that it is indeed a struct

	auto type = jitify_type(original);
	auto struct_type = reinterpret_cast <gcc_jit_struct*> (type);
	auto field = gcc_jit_struct_get_field(struct_type, index);

	auto v = values.at(src);
	auto rv = reinterpret_cast <gcc_jit_rvalue *> (v);
	rv = gcc_jit_rvalue_access_field(rv, LOCATION(context), field);

	return gcc_jit_rvalue_as_object(rv);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Swizzle &swizzle, index_t index)
{
	return load_field(swizzle.src, swizzle.code, false);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Load &load, index_t index)
{
	auto v = values.at(load.src);
	if (load.idx == -1)
		return v;
	
	return load_field(load.src, load.idx, false);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Operation &operation, index_t index)
{
	JVL_ASSERT(operation.b != -1, "unary operations are not implemented");

	auto type = jitify_type(types[index]);
	auto one = reinterpret_cast <gcc_jit_rvalue *> (values.at(operation.a));
	auto two = reinterpret_cast <gcc_jit_rvalue *> (values.at(operation.b));

	return generate_operation(context, operation.code, type, one, two);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Intrinsic &intrinsic, index_t index)

{
	auto type = jitify_type(types[index]);

	std::vector <PrimitiveType> argument_types;
	std::vector <gcc_jit_rvalue *> argument_rvalues;

	index_t next = intrinsic.args;
	while (next != -1) {
		auto &atom = atoms[next];
		JVL_ASSERT(atom.is <List> (),
			"intrinsic arguments must be in "
			"the form of list chains, instead got: {}",
			atom);

		auto &list = atom.as <List> ();

		auto rv = reinterpret_cast <gcc_jit_rvalue *> (values.at(list.item));
		auto qt = types[list.item];

		JVL_ASSERT(qt.is_primitive(), "intrinsic arguments must be primitives");

		auto primitive = qt.as <PlainDataType> ().as <PrimitiveType> ();

		argument_types.push_back(primitive);
		argument_rvalues.push_back(rv);

		next = list.next;
	}

	auto info = intrinsic_lookup_info {
		.opn = intrinsic.opn,
		.types = argument_types,
		.returns = type
	};

	auto ftn = intrinsic_lookup(context, info);
	auto expr = gcc_jit_context_new_call(context, LOCATION(context), ftn,
		argument_rvalues.size(), argument_rvalues.data());

	return gcc_jit_rvalue_as_object(expr);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const List &, index_t)
{
	// Nothing to do here...
	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Returns &returns, index_t index)
{
	auto v = values.at(returns.value);
	auto rv = reinterpret_cast <gcc_jit_rvalue *> (v);
	gcc_jit_block_end_with_return(block, LOCATION(context), rv);
	return nullptr;
}

// Expand generation list
auto gcc_jit_function_generator_t::work_list()
{
	auto used = std::unordered_set <index_t> ();

	std::queue <index_t> work;
	for (auto i : synthesized)
		work.push(i);

	while (work.size()) {
		index_t next = work.front();
		work.pop();

		if (next == -1)
			continue;

		auto &atom = atoms[next];	
		if (atom.is <TypeInformation> () && !synthesized.contains(next)) {
			// Unless explicitly requires from the
			// synthesized list of atoms, we
			// should not be generating more types
			continue;
		}

		used.insert(next);

		auto &&addrs = atom.addresses();
		work.push(addrs.a0);
		work.push(addrs.a1);
	}

	return used;
}

// Configure the function
void gcc_jit_function_generator_t::begin_function()
{
	// Pre-processing; get parameters and return type
	parameters.clear();

	gcc_jit_type *return_type = nullptr;

	for (int i = 0; i < pointer; i++) {
		if (!synthesized.count(i))
			continue;

		auto &atom = atoms[i];
		if (auto qualifier = atom.get <Qualifier> ()) {
			if (qualifier->kind == parameter) {
				index_t underlying = qualifier->underlying;
				gcc_jit_type *type = jitify_type(types[underlying]);

				size_t loc = qualifier->numerical;
				size_t size = std::max(parameters.size(), loc + 1);
				parameters.resize(size);

				std::string name = fmt::format("_arg{}", loc);

				parameters[loc] = gcc_jit_context_new_param(context,
					LOCATION(context), type, name.c_str());
			}
		}

		if (auto returns = atom.get <Returns> ())
			return_type = jitify_type(types[i]);
	}

	function = gcc_jit_context_new_function(context,
		LOCATION(context), GCC_JIT_FUNCTION_EXPORTED,
		return_type, "function",
		parameters.size(), parameters.data(), 0);

	block = gcc_jit_function_new_block(function, "primary");
}

// Wholistic generation
void gcc_jit_function_generator_t::generate(index_t i)
{
	auto ftn = [&](auto atom) { return generate(atom, i); };
	auto rv = std::visit(ftn, atoms[i]);
	if (rv)
		values[i] = rv;
}

void gcc_jit_function_generator_t::generate()
{
	begin_function();

	auto used = work_list();
	for (int i = 0; i < pointer; i++) {
		if (used.count(i))
			generate(i);
	}
}

} // namespace detail

Linkage::jit_result_t Linkage::generate_jit_gcc()
{
	JVL_INFO("compiling linkage atoms with gcc jit");

	gcc_jit_context *context = gcc_jit_context_acquire();
	JVL_ASSERT(context, "failed to acquire context");

	// TODO: pass options
	gcc_jit_context_set_int_option(context, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 0);
	gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DEBUGINFO, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_TREE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_SUMMARY, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, true);

	// for (auto &[_, block] : blocks)
	// 	generate_block(context, block);

	for (auto &[_, block] : blocks) {
		// detail::unnamed_body_t body(block);
		detail::gcc_jit_function_generator_t generator(context, block);
		generator.generate();
	}

	gcc_jit_context_dump_to_file(context, "gcc_jit_result.c", true);

	gcc_jit_result *result = gcc_jit_context_compile(context);
	JVL_ASSERT(result, "failed to compile function");

	void *ftn = gcc_jit_result_get_code(result, "function");
	JVL_ASSERT(result, "failed to load function result");

	return { ftn };
}

} // namespace jvl::thunder