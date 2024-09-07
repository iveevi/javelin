#include <queue>

#include <libgccjit.h>

#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "logging.hpp"

namespace jvl::thunder {

MODULE(gcc-jit);

struct jit_struct {
	// TODO: header with eightcc and hash for cleanup
	gcc_jit_type *type;
	std::vector <gcc_jit_field *> fields;
	std::vector <jit_struct *> field_types;

	jit_struct &materialize(gcc_jit_context *context, const std::string &name) {
		gcc_jit_struct *struct_type = gcc_jit_context_new_struct_type(context,
			nullptr,
			name.c_str(),
			fields.size(),
			fields.data());

		type = gcc_jit_struct_as_type(struct_type);

		return *this;
	}

	gcc_jit_rvalue *construct(gcc_jit_context *context, std::vector <gcc_jit_rvalue *> &args) {
		return gcc_jit_context_new_struct_constructor(context,
			nullptr,
			type,
			args.size(),
			fields.data(),
			args.data());
	}
};

struct jit_instruction {
	gcc_jit_object *value;
	jit_struct *type_info;

	static auto null() {
		return jit_instruction(nullptr, nullptr);
	}
};

struct jit_context {
	const std::vector <Atom> &pool;

	gcc_jit_context *const gcc;
	gcc_jit_function *function;
	gcc_jit_block *block;

	std::vector <jit_instruction> parameters;
	std::vector <jit_instruction> cached;

	wrapped::hash_table <PrimitiveType, jit_struct *> primitive_types;
};

jit_struct *generate_type_field_primitive_scalar(jit_context &context, PrimitiveType item)
{
	jit_struct *s = new jit_struct;

	switch (item) {
	case i32:
		s->type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_INT32_T);
		break;
	case u32:
		s->type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_UINT32_T);
		break;
	case f32:
		s->type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_FLOAT);
		break;
	default:
		JVL_ABORT("unsupported primitive type {} encountered in {}",
			tbl_primitive_types[item], __FUNCTION__);
	}

	return s;
}

jit_struct *generate_type_field_primitive_vector(jit_context &context,
						 const char *name,
						 PrimitiveType item,
						 size_t components)
{
	static constexpr const char *component_names[] = { "x", "y", "z", "w" };

	// Skip generation if its already cached
	if (auto sptr = context.primitive_types.get(item))
		return sptr.value();

	jit_struct *s = new jit_struct;

	jit_struct *element = generate_type_field_primitive_scalar(context, item);

	s->fields.resize(components);
	for (size_t i = 0; i < components; i++) {
		s->fields[i] = gcc_jit_context_new_field(context.gcc, nullptr,
			element->type, component_names[i]);
	}

	s->field_types.resize(components);
	for (size_t i = 0; i < components; i++)
		s->field_types[i] = element;

	fmt::println("creating a new struct from primitive {}", tbl_primitive_types[item]);
	s->materialize(context.gcc, name);
	fmt::println("struct type: {}", (void *) s->type);
	context.primitive_types[item] = s;
	return s;
}

jit_struct *generate_type_field_primitive(jit_context &context, PrimitiveType item)
{
	switch (item) {

	// Scalar types
	case i32:
	case u32:
	case f32:
		return generate_type_field_primitive_scalar(context, item);

	// Integer vector types
	case ivec2:
		return generate_type_field_primitive_vector(context, "ivec2", i32, 2);
	case ivec3:
		return generate_type_field_primitive_vector(context, "ivec3", i32, 3);
	case ivec4:
		return generate_type_field_primitive_vector(context, "ivec4", i32, 4);

	// Unsigned integer vector types
	case uvec2:
		return generate_type_field_primitive_vector(context, "uvec2", u32, 2);
	case uvec3:
		return generate_type_field_primitive_vector(context, "uvec3", u32, 3);
	case uvec4:
		return generate_type_field_primitive_vector(context, "uvec4", u32, 4);

	// Floating-point vector types
	case vec2:
		return generate_type_field_primitive_vector(context, "vec2", f32, 2);
	case vec3:
		return generate_type_field_primitive_vector(context, "vec3", f32, 3);
	case vec4:
		return generate_type_field_primitive_vector(context, "vec4", f32, 4);

	default:
		JVL_ABORT("unsupported primitive type {} encountered in {}",
			tbl_primitive_types[item], __FUNCTION__);
	}

	return nullptr;
}

jit_struct *generate_type_field(jit_context &context, index_t i)
{
	auto &atom = context.pool[i];
	assert(atom.is <TypeField> ());

	TypeField tf = atom.as <TypeField> ();
	if (tf.next == -1 && tf.down == -1)
		return generate_type_field_primitive(context, tf.item);

	// Assume to be a struct aggregrate type
	jit_struct *s = new jit_struct;

	fmt::println("new struct type: {}", (void *) s);
	fmt::println("  for type field: {}", atom);

	while (i != -1) {
		auto &atom = context.pool[i];
		assert(atom.is <TypeField> ());

		TypeField tf = atom.as <TypeField> ();

		gcc_jit_type *field_type = nullptr;

		if (tf.down == -1) {
			fmt::println("  field for primitive: {}", tbl_primitive_types[tf.item]);
			jit_struct *sf = generate_type_field_primitive(context, tf.item);
			s->field_types.push_back(sf);
			field_type = sf->type;
		} else {
			log::abort(__module__, "nested aggregates are currently unsupported");
		}

		size_t fn = s->fields.size();
		std::string fname = fmt::format("f{}", fn);
		gcc_jit_field *field = gcc_jit_context_new_field(context.gcc, nullptr,
			field_type, fname.c_str());

		s->fields.push_back(field);

		i = tf.next;
	}

	fmt::println("creating a new struct with {} fields", s->fields.size());
	s->materialize(context.gcc, fmt::format("s_"));
	fmt::println("struct type: {}", (void *) s->type);
	return s;
}

std::vector <gcc_jit_rvalue *> load_rvalue_arguments(jit_context &context, index_t argsi)
{
	std::vector <gcc_jit_rvalue *> args;

	index_t next = argsi;
	while (next != -1) {
		auto &g = context.pool[next];
		assert(g.is <List> ());

		auto &list = g.as <List> ();

		// TODO: Some instructions will emit an l-value, so need to watch out
		args.push_back((gcc_jit_rvalue *) context.cached[list.item].value);

		next = list.next;
	}

	return args;
}

jit_instruction generate_instruction_primitive(jit_context &context, const Primitive &primitive)
{
	switch (primitive.type) {

	case i32:
	{
		jit_instruction ji;
		ji.type_info = generate_type_field_primitive(context, i32);
		ji.value = (gcc_jit_object *) gcc_jit_context_new_rvalue_from_int(context.gcc,
				ji.type_info->type, primitive.idata);
		return ji;
	}

	case u32:
	{
		jit_instruction ji;
		ji.type_info = generate_type_field_primitive(context, u32);
		ji.value = (gcc_jit_object *) gcc_jit_context_new_rvalue_from_int(context.gcc,
				ji.type_info->type, primitive.udata);
		return ji;
	}

	case f32:
	{
		jit_instruction ji;
		ji.type_info = generate_type_field_primitive(context, f32);
		ji.value = (gcc_jit_object *) gcc_jit_context_new_rvalue_from_double(context.gcc,
				ji.type_info->type, primitive.fdata);
		return ji;
	}

	default:
		break;
	}

	fmt::println("GCC JIT unexpected primitive: {}", primitive);
	abort();
}

jit_instruction generate_instruction_binary_operation(jit_context &context,
						      OperationCode code,
						      jit_struct *return_type,
						      const std::vector <gcc_jit_rvalue *> &args)
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
		gcc_jit_rvalue *expr = gcc_jit_context_new_binary_op(context.gcc,
			nullptr, gcc_code.value(),
			return_type->type, args[0], args[1]);

		assert(expr);

		jit_instruction ji;
		ji.value = (gcc_jit_object *) expr;
		ji.type_info = return_type;
		return ji;
	}

	fmt::println("GCC JIT unsupported operation {}", tbl_operation_code[code]);
	abort();
}

jit_instruction generate_instruction_intrinsic(jit_context &context,
					       IntrinsicOperation opn,
					       jit_struct *return_type,
					       const std::vector <gcc_jit_rvalue *> &args)
{
	switch (opn) {

	case sin:
	{
		gcc_jit_function *intr_ftn = gcc_jit_context_get_builtin_function(context.gcc, "sin");
		assert(intr_ftn);

		// Casting the arguments
		gcc_jit_type *double_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_DOUBLE);
		gcc_jit_type *float_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_FLOAT);
		assert(double_type);
		assert(float_type);

		gcc_jit_rvalue *arg = gcc_jit_context_new_cast(context.gcc, nullptr, args[0], double_type);
		assert(arg);

		gcc_jit_rvalue *expr;
		expr = gcc_jit_context_new_call(context.gcc, nullptr, intr_ftn, 1, &arg);
		expr = gcc_jit_context_new_cast(context.gcc, nullptr, expr, float_type);

		jit_instruction ji;
		ji.value = (gcc_jit_object *) expr;
		ji.type_info = return_type;
		return ji;
	}

	case cos:
	{
		gcc_jit_function *intr_ftn = gcc_jit_context_get_builtin_function(context.gcc, "cos");
		assert(intr_ftn);

		// Casting the arguments
		gcc_jit_type *double_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_DOUBLE);
		gcc_jit_type *float_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_FLOAT);
		assert(double_type);
		assert(float_type);

		gcc_jit_rvalue *arg = gcc_jit_context_new_cast(context.gcc, nullptr, args[0], double_type);
		assert(arg);

		gcc_jit_rvalue *expr;
		expr = gcc_jit_context_new_call(context.gcc, nullptr, intr_ftn, 1, &arg);
		expr = gcc_jit_context_new_cast(context.gcc, nullptr, expr, float_type);

		jit_instruction ji;
		ji.value = (gcc_jit_object *) expr;
		ji.type_info = return_type;
		return ji;
	}

	// Intrinsics which could be supported if they had been lowered properly
	case dot:
		JVL_ABORT("intrinsic instruction ${} must be lowered", tbl_intrinsic_operation[opn]);
		return jit_instruction::null();

	default:
		break;
	}

	JVL_ABORT("unsupported intrinsic instruction ${}", tbl_intrinsic_operation[opn]);
	return jit_instruction::null();
}

jit_instruction generate_instruction_access_field(jit_context &context, index_t src, index_t idx)
{
	auto &atom = context.pool[src];

	jit_instruction source_instruction = context.cached[src];
	jit_struct *type_info = source_instruction.type_info;

	JVL_ASSERT(idx < type_info->field_types.size(),
		"index {} is out of bounds of jit_struct ({} fields, @{})",
		idx, type_info->field_types.size(), (void *) type_info);

	gcc_jit_field *field = type_info->fields[idx];

	// Handle special cases of loading
	gcc_jit_rvalue *source = nullptr;

	switch (atom.index()) {

	// TODO: currently assuming its a parameter
	// Convert the cached instruction value to an r-value
	case Atom::type_index <Global> ():
	{
		fmt::println("loading from a global/parameter");
		gcc_jit_param *parameter = (gcc_jit_param *) source_instruction.value;
		source = gcc_jit_param_as_rvalue(parameter);
	} break;

	default:
		source = (gcc_jit_rvalue *) source_instruction.value;
		break;
	}

	gcc_jit_rvalue *expr = gcc_jit_rvalue_access_field(source, nullptr, field);

	jit_instruction ji;
	ji.type_info = type_info->field_types[idx];
	ji.value = (gcc_jit_object *) expr;
	return ji;
}

[[gnu::always_inline]]
inline jit_instruction generate_instruction_access_field(jit_context &context, const Load &load)
{
	return generate_instruction_access_field(context, load.src, load.idx);
}

[[gnu::always_inline]]
inline jit_instruction generate_instruction_access_field(jit_context &context, const Swizzle &swizzle)
{
	// TODO: split up if necessary
	return generate_instruction_access_field(context, swizzle.src, swizzle.code);
}

jit_instruction generate_instruction_store(jit_context &context, const Store &store)
{
	// Determine the access chain used to reach the lvalue
	std::queue <index_t> access_chain_indices;
	std::queue <gcc_jit_field *> access_chain_fields;

	fmt::println("finding access chain for store");

	index_t idx = store.dst;
	index_t dst = -1;
	while (dst == -1) {
		auto &atom = context.pool[idx];

		fmt::println("  {}", atom);

		// Valid paths can only include loads and swizzles
		if (auto load = atom.get <Load> ()) {
			auto &source_type = context.cached[load->src].type_info;
			auto &loaded_field = source_type->fields[load->idx];

			fmt::println("  through load");
			access_chain_indices.push(load->idx);
			access_chain_fields.push(loaded_field);
			idx = load->src;
		} else if (auto swizzle = atom.get <Swizzle> ()) {
			auto &source_type = context.cached[swizzle->src].type_info;
			auto &loaded_field = source_type->fields[swizzle->code];

			fmt::println("  through swizzle");
			access_chain_indices.push(swizzle->code);
			access_chain_fields.push(loaded_field);
			idx = swizzle->src;
		} else {
			// Otherwise we have found the final location
			fmt::println("final destination of access chain");
			dst = idx;
			break;
		}
	}

	assert(dst != -1);

	auto copy = access_chain_indices;
	fmt::print("final access chain: ");
	while (copy.size()) {
		fmt::print(" {}", copy.front());
		copy.pop();
	}
	fmt::print("\n");
	fmt::println("with destination: {}", context.pool[dst]);

	// Value side
	gcc_jit_rvalue *rvalue = (gcc_jit_rvalue *) context.cached[store.src].value;

	// TODO: handling for lvalues
	gcc_jit_object *object = context.cached[dst].value;
	gcc_jit_lvalue *lvalue = nullptr;

	auto &destination = context.pool[dst];
	switch (destination.index()) {
	case Atom::type_index <Global> ():
		lvalue = gcc_jit_param_as_lvalue((gcc_jit_param *) object);
		break;
	default:
		break;
	}

	if (!lvalue) {
		fmt::println("GCC JIT failed to extract l-value from instruction: {}", destination);
		abort();
	}

	fmt::println("got l-value: {}", (void *) lvalue);
	while (access_chain_fields.size()) {
		gcc_jit_field *field = access_chain_fields.front();
		access_chain_fields.pop();
		lvalue = gcc_jit_lvalue_access_field(lvalue, nullptr, field);
		fmt::println("  next l-value: {}", (void *) lvalue);
	}

	fmt::println("final l-value: {}", (void *) lvalue);
	gcc_jit_block_add_assignment(context.block, nullptr, lvalue, rvalue);

	// Stores should never be used directly
	return { nullptr, nullptr };
}

jit_instruction generate_instruction(jit_context &context, index_t i)
{
	auto &atom = context.pool[i];

	fmt::println("> processing atom: {}", atom);

	// TODO: skip if not marked for synthesis

	switch (atom.index()) {

	case Atom::type_index <Global> ():
	{
		auto &global = atom.as <Global> ();
		assert(global.qualifier == parameter);
		return context.parameters[global.binding];
	}

	case Atom::type_index <TypeField> ():
	{
		// Skip if already generated by the parameters or etc.
		if (context.cached[i].type_info)
			return context.cached[i];

		jit_instruction ji;
		ji.value = nullptr;
		ji.type_info = generate_type_field(context, i);
		return ji;
	}

	case Atom::type_index <Primitive> ():
	{
		auto &primitive = atom.as <Primitive> ();
		return generate_instruction_primitive(context, primitive);
	}

	case Atom::type_index <Construct> ():
	{
		auto &constructor = atom.as <Construct> ();
		auto args = load_rvalue_arguments(context, constructor.args);
		jit_struct *struct_type = context.cached[constructor.type].type_info;

		jit_instruction ji;
		ji.type_info = struct_type;
		ji.value = (gcc_jit_object *) struct_type->construct(context.gcc, args);
		return ji;
	}

	case Atom::type_index <Operation> ():
	{
		auto &opn = atom.as <Operation> ();
		auto args = load_rvalue_arguments(context, opn.args);
		auto return_type = context.cached[opn.type].type_info;
		return generate_instruction_binary_operation(context, opn.code, return_type, args);
	}

	case Atom::type_index <Load> ():
		return generate_instruction_access_field(context, atom.as <Load> ());

	case Atom::type_index <Swizzle> ():
		return generate_instruction_access_field(context, atom.as <Swizzle> ());

	case Atom::type_index <Store> ():
		return generate_instruction_store(context, atom.as <Store> ());

	case Atom::type_index <List> ():
		// Skip lists, they are only for the Thunder IR
		return jit_instruction::null();

	case Atom::type_index <Returns> ():
	{
		auto &returns = atom.as <Returns> ();
		auto args = load_rvalue_arguments(context, returns.args);
		assert(args.size() == 1);
		gcc_jit_block_end_with_return(context.block, nullptr, args[0]);
	} return jit_instruction::null();

	case Atom::type_index <Intrinsic> ():
	{
		auto &intr = atom.as <Intrinsic> ();
		auto args = load_rvalue_arguments(context, intr.args);
		auto return_type = context.cached[intr.type].type_info;
		return generate_instruction_intrinsic(context, intr.opn, return_type, args);
	}

	default:
		break;
	}

	fmt::println("GCC JIT unsupported instruction: {}", atom.to_string());
	abort();
}

void generate_block(gcc_jit_context *const gcc, Linkage::block_t &block)
{
	fmt::println("generating block");

	jit_context context {
		.pool = block.unit,
		.gcc = gcc,
	};

	// JIT the parameters
	std::vector <gcc_jit_param *> gcc_parameters;

	gcc_parameters.resize(block.parameters.size());
	context.parameters.resize(block.parameters.size());
	context.cached.resize(block.unit.size(), jit_instruction::null());

	for (auto &[i, t] : block.parameters) {
		fmt::println("\nGENERATING PARAMETER ARG{} (type {})", i, t);
		
		std::string name = fmt::format("_arg{}", i);

		jit_struct *type_info = generate_type_field(context, t);
		gcc_jit_param *parameter = gcc_jit_context_new_param(context.gcc, nullptr, type_info->type, name.c_str());

		jit_instruction ji;
		ji.type_info = type_info;
		ji.value = (gcc_jit_object *) parameter;

		gcc_parameters[i] = parameter;
		context.parameters[i] = ji;

		// Save the generated type into the cache
		ji.value = nullptr;
		context.cached[t] = ji;
	}
	
	// Determine the return type
	gcc_jit_type *return_type = nullptr;

	if (auto ptr = context.cached[block.returns].type_info) {
		return_type = ptr->type;
	} else {
		fmt::println("block returns value: {} (ret: {})", block.unit[block.returns], block.returns);
		return_type = generate_type_field(context, block.returns)->type;
		fmt::println("return type: @{}", (void *) return_type);
	}

	// Begin the function context
	// TODO: embed the scope name in the block

	context.function = gcc_jit_context_new_function(context.gcc, nullptr,
		GCC_JIT_FUNCTION_EXPORTED, return_type, "function",
		context.parameters.size(), gcc_parameters.data(), 0);

	// TODO: distinguish between function_t and block_t

	context.block = gcc_jit_function_new_block(context.function, nullptr);

	// TODO: cache block synthesized status
	auto synthesized = detail::synthesize_list(block.unit);

	// Do some extra processing to get the list of required instructions
	auto used = decltype(synthesized)();

	std::queue <index_t> work;
	for (auto i : synthesized)
		work.push(i);

	while (work.size()) {
		index_t next = work.front();
		work.pop();

		if (next == -1)
			continue;

		auto &atom = block.unit[next];	
		if (atom.is <TypeField> () && !synthesized.contains(next)) {
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

	// Finally, generate the instructions in JIT
	for (index_t i = 0; i < block.unit.size(); i++) {
		if (used.contains(i))
			context.cached[i] = generate_instruction(context, i);

		fmt::println("$ cached @{} -> {}, {}", i,
			(void *) context.cached[i].value,
			(void *) context.cached[i].type_info);
	}
}

Linkage::jit_result_t Linkage::generate_jit_gcc()
{
	dump();

	fmt::println("compiling linkage unit with gcc jit");

	gcc_jit_context *context = gcc_jit_context_acquire();
	JVL_ASSERT(context, "failed to acquire context");

	// TODO: pass options
	gcc_jit_context_set_int_option(context, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 0);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_TREE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_SUMMARY, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, true);

	for (auto &[_, block] : blocks)
		generate_block(context, block);

	gcc_jit_result *result = gcc_jit_context_compile(context);
	JVL_ASSERT(result, "failed to compile function");

	void *ftn = gcc_jit_result_get_code(result, "function");
	JVL_ASSERT(result, "failed to load function result");

	return { ftn };
}

} // namespace jvl::thunder
