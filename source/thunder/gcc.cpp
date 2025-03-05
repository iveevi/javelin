#include <queue>

#include <dlfcn.h>

#include <libgccjit.h>

#include "common/logging.hpp"

#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/qualified_type.hpp"
#include "thunder/properties.hpp"
#include "thunder/gcc_jit_generator.hpp"

// Intrinsic implementations
// TODO: separate header file
extern "C" float clamp(float x, float low, float high)
{
	return std::min(high, std::max(low, x));
}

namespace jvl::thunder::detail {

MODULE(gcc-jit);

#define LOCATION(context) gcc_jit_context_new_location(context, __FILE__, __LINE__, 0)

gcc_type_info generate_primitive_scalar_type(gcc_jit_context *const context, PrimitiveType item)
{
	switch (item) {
	case boolean:
		return {
			.real = gcc_jit_context_get_type(context, GCC_JIT_TYPE_BOOL),
			.size = 4,
			.align = 4,
		};
	case i32:
		return {
			.real = gcc_jit_context_get_type(context, GCC_JIT_TYPE_INT32_T),
			.size = 4,
			.align = 4,
		};
	case u32:
		return {
			.real = gcc_jit_context_get_type(context, GCC_JIT_TYPE_UINT32_T),
			.size = 4,
			.align = 4,
		};
	case f32:
		return {
			.real = gcc_jit_context_get_type(context, GCC_JIT_TYPE_FLOAT),
			.size = 4,
			.align = 4,
		};
	default:
		break;
	}

	JVL_ABORT("unsupported primitive type {} encountered in {}",
		tbl_primitive_types[item], __FUNCTION__);
}

gcc_type_info generate_primitive_vector_type(gcc_jit_context *context,
					     const char *name,
					     PrimitiveType item,
					     uint32_t components)
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
			nullptr,
			element.real,
			component_names[i]);
	}

	gcc_jit_struct *vector = gcc_jit_context_new_struct_type(context,
		LOCATION(context),
		name,
		fields.size(),
		fields.data());

	JVL_INFO("created struct for vector "
		"primitive type {}x{}",
		tbl_primitive_types[item], components);

	// TODO: generalize
	return {
		.real = gcc_jit_struct_as_type(vector),
		.size = components * element.size,
		.align = (components == 3) ? 4 * element.size : components * element.size
	};
}

gcc_type_info generate_primitive_type(gcc_jit_context *const context, PrimitiveType item)
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
		return generate_primitive_vector_type(context, "ivec2", i32, 2);
	case ivec3:
		return generate_primitive_vector_type(context, "ivec3", i32, 3);
	case ivec4:
		return generate_primitive_vector_type(context, "ivec4", i32, 4);

	// Unsigned integer vector types
	case uvec2:
		return generate_primitive_vector_type(context, "uvec2", u32, 2);
	case uvec3:
		return generate_primitive_vector_type(context, "uvec3", u32, 3);
	case uvec4:
		return generate_primitive_vector_type(context, "uvec4", u32, 4);

	// Floating-point vector types
	case vec2:
		return generate_primitive_vector_type(context, "vec2", f32, 2);
	case vec3:
		return generate_primitive_vector_type(context, "vec3", f32, 3);
	case vec4:
		return generate_primitive_vector_type(context, "vec4", f32, 4);

	default:
		break;
	}

	JVL_ABORT("unsupported primitive type {} encountered in {}",
		tbl_primitive_types[item], __FUNCTION__);
}

gcc_jit_object *generate_primitive(gcc_jit_context *const context, const Primitive &primitive)
{
	auto scalar_type = generate_primitive_scalar_type(context, primitive.type).real;

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
	static const bestd::hash_table <OperationCode, gcc_jit_binary_op> table {
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
			
		static const bestd::hash_table <IntrinsicOperation, const char *const> float_names {
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

	JVL_ABORT("{} intrinsic is unsupported in (gcc) JIT", tbl_intrinsic_operation[info.opn]);
}

gcc_jit_function_generator_t::gcc_jit_function_generator_t(gcc_jit_context *const context_, const Buffer &buffer_)
		: Buffer(buffer_), context(context_) {}

// Generating types
gcc_type_info gcc_jit_function_generator_t::jitify_type(QualifiedType qt)
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

		Index concrete = pd.as <Index> ();
		return jitify_type(types[concrete]);
	} break;

	variant_case(QualifiedType, StructFieldType):
	{
		std::vector <gcc_type_info> field_infos;
			
		fmt::println("creating new structure...");

		while (!qt.is <NilType> ()) {
			fmt::println("field: {}", qt);

			gcc_type_info info;
			if (qt.is <StructFieldType> ()) {
				auto &sft = qt.as <StructFieldType> ();
				info = jitify_type(sft.base());

				Index next = qt.as <StructFieldType> ().next;
				qt = types[next];
			} else {
				info = jitify_type(qt);

				qt = NilType();
			}

			fmt::println("  size of resulting field: {}", info.size);
			fmt::println("  align of resulting field: {}", info.align);

			field_infos.push_back(info);
		}

		auto aligned = [](uint32_t offset, uint32_t bytes) {
			return bytes * ((offset + bytes - 1) / bytes);
		};

		uint32_t offset = 0;
		for (size_t i = 0; i < field_infos.size(); i++) {
			auto &current = field_infos[i];

			fmt::println("adding field (size={}, align={}, offset={})",
				current.size,
				current.align,
				offset);

			if (offset % current.align) {
				fmt::println("  aligning to {} bytes", current.align);
				current.real = gcc_jit_type_get_aligned(current.real, current.align);
				offset = aligned(offset, current.align) + current.size;
			} else {
				// Keep the alignment
				offset += current.size;
			}

			// offset += current.size;
			// // Now check with the alignment of the next field
			// if (i + 1 < field_infos.size()) {
			// 	auto &next = field_infos[i + 1];

			// 	uint32_t rounded = aligned(offset, next.align);
			// 	fmt::println("  rounded size: {} vs offset: {}", rounded, offset);
			// 	if (rounded != offset) {
			// 		fmt::println("  aligning to {} bytes", next.align);
			// 		current.real = gcc_jit_type_get_aligned(current.real, next.align);
			// 	}
			// }
		}

		std::vector <gcc_jit_field *> fields;
		for (size_t i = 0; i < field_infos.size(); i++) {
			auto &current = field_infos[i];

			std::string name = fmt::format("f{}", i);
			auto field = gcc_jit_context_new_field(context,
				LOCATION(context),
				current.real,
				name.c_str());

			fields.push_back(field);
		}

		fmt::println("total # of fields: {}", fields.size());

		std::string name = fmt::format("s{}", mapped_types.size());
		auto structure = gcc_jit_context_new_struct_type(context,
			LOCATION(context),
			name.c_str(),
			fields.size(),
			fields.data());

		auto type = gcc_jit_struct_as_type(structure);
		auto info = gcc_type_info {
			.real = type,
			.size = offset,
			// TODO: this is hard coded
			.align = 16
		};

		mapped_types[original] = info;

		return info;
	}

	default:
		break;
	}

	JVL_ABORT("failed to JIT (gcc-jit) the following type: {}", original);
}

// Expanding list chains
gcc_jit_function_generator_t::expanded_list_chain gcc_jit_function_generator_t::expand_list_chain(Index next) const
{
	std::vector <PrimitiveType> argument_types;
	std::vector <gcc_jit_rvalue *> argument_rvalues;

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

	return { argument_types, argument_rvalues };
}

// Per-atom generator
template <typename T>
gcc_jit_object *generate(const T &atom, Index i) {
	JVL_ABORT("failed to JIT (gcc-jit) compile atom: {} (@{})", atom, i);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Qualifier &, Index)
{
	// Nothing to do here...
	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const TypeInformation &, Index index)
{
	auto &qt = types[index];

	// All we need to do it generate the type so it is ready
	jitify_type(qt);

	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Primitive &primivite, Index index)
{
	return generate_primitive(context, primivite);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Construct &constructor, Index index)
{
	// Handle the special case of transient constructors
	if (constructor.mode == global) {
		auto &atom = atoms[constructor.type];
		JVL_ASSERT(atom.is <Qualifier> (), "transient constructors must be typed to qualifiers");

		auto &qualifier = atom.as <Qualifier> ();
		if (qualifier.kind == parameter) {
			Index loc = qualifier.numerical;
			auto rv = gcc_jit_param_as_rvalue(parameters[loc]);
			return gcc_jit_rvalue_as_object(rv);
		}

		JVL_ABORT("transient construction of {} "
			"qualified types is unsupported",
			tbl_qualifier_kind[qualifier.kind]);
	}

	// Regular constructors
	auto args = expand_list_chain(constructor.args);

	auto info = jitify_type(types[index]);

	fmt::println("regular constructor: {} args", args.rvalues.size());
	fmt::println("  struct size: {}", info.size);

	gcc_jit_type *type = info.real;
	gcc_jit_struct *structure = reinterpret_cast <gcc_jit_struct *> (type);

	uint32_t count = gcc_jit_struct_get_field_count(structure);

	JVL_ASSERT(count == args.rvalues.size(),
		"mismatch in construct arguments (got {}, expected {}), "
		"did you legalize the instructions for GCC JIT?",
		args.rvalues.size(), count);
	
	// Requires a one-to-one mapping from args to fields
	std::vector <gcc_jit_field *> fields;
	for (size_t i = 0; i < args.rvalues.size(); i++) {
		auto field = gcc_jit_struct_get_field(structure, i);
		fields.emplace_back(field);
	}

	gcc_jit_rvalue *constructed = gcc_jit_context_new_struct_constructor(context,
		LOCATION(context),
		type,
		args.rvalues.size(),
		fields.data(),
		args.rvalues.data());

	JVL_ASSERT_PLAIN(constructed);

	return gcc_jit_rvalue_as_object(constructed);
}

gcc_jit_object *gcc_jit_function_generator_t::load_field(Index src, Index index, bool lvalue)
{
	JVL_ASSERT(!lvalue, "lvalue loading is not implemented");

	// Find the original type
	QualifiedType original = types[src];
	// TODO: is_aggregate method()
	// TODO: double check that it is indeed a struct

	auto type = jitify_type(original);
	auto struct_type = reinterpret_cast <gcc_jit_struct*> (type.real);
	auto field = gcc_jit_struct_get_field(struct_type, index);

	auto v = values.at(src);
	auto rv = reinterpret_cast <gcc_jit_rvalue *> (v);
	rv = gcc_jit_rvalue_access_field(rv, LOCATION(context), field);

	return gcc_jit_rvalue_as_object(rv);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Swizzle &swizzle, Index index)
{
	return load_field(swizzle.src, swizzle.code, false);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Store &store, Index index)
{
	auto dst = reinterpret_cast <gcc_jit_lvalue *> (values.at(store.dst));
	auto src = reinterpret_cast <gcc_jit_rvalue *> (values.at(store.src));
	gcc_jit_block_add_assignment(block, LOCATION(context), dst, src);
	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Load &load, Index index)
{
	auto v = values.at(load.src);
	if (load.idx == -1)
		return v;
	
	return load_field(load.src, load.idx, false);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Operation &operation, Index index)
{
	JVL_ASSERT(operation.b != -1, "unary operations are not implemented");

	auto type = jitify_type(types[index]);
	auto one = reinterpret_cast <gcc_jit_rvalue *> (values.at(operation.a));
	auto two = reinterpret_cast <gcc_jit_rvalue *> (values.at(operation.b));

	return generate_operation(context, operation.code, type.real, one, two);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Intrinsic &intrinsic, Index index)

{
	auto type = jitify_type(types[index]);

	auto args = expand_list_chain(intrinsic.args);

	auto info = intrinsic_lookup_info {
		.opn = intrinsic.opn,
		.types = args.types,
		.returns = type.real
	};

	auto ftn = intrinsic_lookup(context, info);
	auto expr = gcc_jit_context_new_call(context, LOCATION(context), ftn,
		args.rvalues.size(), args.rvalues.data());

	return gcc_jit_rvalue_as_object(expr);
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const List &, Index)
{
	// Nothing to do here...
	return nullptr;
}

template <>
gcc_jit_object *gcc_jit_function_generator_t::generate(const Return &returns, Index index)
{
	auto v = values.at(returns.value);
	auto rv = reinterpret_cast <gcc_jit_rvalue *> (v);
	gcc_jit_block_end_with_return(block, LOCATION(context), rv);
	return nullptr;
}

// Expand generation list
auto gcc_jit_function_generator_t::work_list()
{
	auto used = std::set <Index> ();

	std::queue <Index> work;
	for (auto i : marked)
		work.push(i);

	while (work.size()) {
		Index next = work.front();
		work.pop();

		if (next == -1)
			continue;

		auto &atom = atoms[next];	
		if (atom.is <TypeInformation> () && !marked.contains(next)) {
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

	for (size_t i = 0; i < pointer; i++) {
		if (!marked.count(i))
			continue;

		auto &atom = atoms[i];
		if (auto qualifier = atom.get <Qualifier> ()) {
			if (qualifier->kind == parameter) {
				Index underlying = qualifier->underlying;
				gcc_jit_type *type = jitify_type(types[underlying]).real;

				size_t loc = qualifier->numerical;
				size_t size = std::max(parameters.size(), loc + 1);
				parameters.resize(size);

				std::string name = fmt::format("_arg{}", loc);

				parameters[loc] = gcc_jit_context_new_param(context,
					LOCATION(context), type, name.c_str());
			}
		}

		if (auto returns = atom.get <Return> ())
			return_type = jitify_type(types[i]).real;
	}

	function = gcc_jit_context_new_function(context,
		LOCATION(context), GCC_JIT_FUNCTION_EXPORTED,
		return_type, "function",
		parameters.size(), parameters.data(), 0);

	block = gcc_jit_function_new_block(function, "primary");
}

// Wholistic generation
void gcc_jit_function_generator_t::generate(Index i)
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
	for (size_t i = 0; i < pointer; i++) {
		if (used.count(i))
			generate(i);
	}
}

} // namespace jvl::thunder::detail