#include <libgccjit.h>

#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"

namespace jvl::thunder {

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
};

struct jit_context {
	gcc_jit_context *const gcc;
	gcc_jit_function *function;
	gcc_jit_block *block;
	std::vector <jit_instruction> parameters;
	std::vector <jit_instruction> cached;
	const std::vector <Atom> &pool;
};

jit_struct *generate_type_field_primitive_scalar(jit_context &context, PrimitiveType item)
{
	jit_struct *s = new jit_struct;

	switch (item) {
	case i32:
		s->type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_INT);
		break;
	case f32:
		s->type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_FLOAT);
		break;
	default:
		fmt::println("GCC JIT unsupported type {}", tbl_primitive_types[item]);
		abort();
	}

	return s;
}

jit_struct *generate_type_field_primitive_vector(jit_context &context,
						 const char *name,
						 PrimitiveType item,
						 size_t components)
{
	static constexpr const char *component_names[] = { "x", "y", "z", "w" };

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

	return &s->materialize(context.gcc, name);
}

jit_struct *generate_type_field_primitive(jit_context &context, PrimitiveType item)
{
	switch (item) {

	// Scalar types
	case i32:
	case f32:
		return generate_type_field_primitive_scalar(context, item);
	
	// Integer vector types
	case ivec2:
		return generate_type_field_primitive_vector(context, "ivec2", i32, 2);
	case ivec3:
		return generate_type_field_primitive_vector(context, "ivec3", i32, 3);
	case ivec4:
		return generate_type_field_primitive_vector(context, "ivec4", i32, 4);
	
	// Floating-point vector types
	case vec2:
		return generate_type_field_primitive_vector(context, "vec2", f32, 2);
	case vec3:
		return generate_type_field_primitive_vector(context, "vec3", f32, 3);
	case vec4:
		return generate_type_field_primitive_vector(context, "vec4", f32, 4);

	default:
		fmt::println("GCC JIT unsupported type {}", tbl_primitive_types[item]);
		abort();
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
			fmt::println("GCC JIT does not supported nested aggregates");
			abort();
		}

		size_t fn = s->fields.size();
		std::string fname = fmt::format("f{}", fn);
		gcc_jit_field *field = gcc_jit_context_new_field(context.gcc, nullptr,
			field_type, fname.c_str());

		s->fields.push_back(field);

		i = tf.next;
	}

	fmt::println("creating a new struct with {} fields", s->fields.size());
	return &s->materialize(context.gcc, fmt::format("s_"));
}

std::vector <gcc_jit_rvalue *> load_rvalue_arguments(jit_context &context, index_t argsi)
{
	std::vector <gcc_jit_rvalue *> args;

	index_t next = argsi;
	while (next != -1) {
		auto &g = context.pool[next];
		assert(g.is <List> ());

		auto &list = g.as <List> ();
		args.push_back((gcc_jit_rvalue *) context.cached[list.item].value);

		next = list.next;
	}

	return args;
}

jit_instruction generate_instruction_primitive(jit_context &context,
					       const Primitive &primitive)
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
		
	fmt::println("GCC JIT unexpected primitive type: {}", primitive);
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
	assert(args.size() == 1);
		
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

	default:
		break;
	}
		
	fmt::println("GCC JIT unsupported intrinsic <{}>", tbl_intrinsic_operation[opn]);
	abort();
}

jit_instruction generate_instruction(jit_context &context, index_t i)
{
	auto &atom = context.pool[i];

	// TODO: skip if not marked for synthesis

	switch (atom.index()) {

	case Atom::type_index <Global> ():
	{
		auto &global = atom.as <Global> ();
		assert(global.qualifier == parameter);
		jit_instruction ji = context.parameters[global.binding];
		ji.value = (gcc_jit_object *) gcc_jit_param_as_rvalue((gcc_jit_param *) ji.value);
		return ji;
	}

	case Atom::type_index <TypeField> ():
	{
		// TODO: skip if already generated by the parameters or etc.
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
	{
		auto &load = atom.as <Load> ();

		jit_instruction source = context.cached[load.src];
		
		jit_struct *type_info = source.type_info;

		fmt::println("load instruction:");
		fmt::println("type_info: {}", (void *) type_info);
		fmt::println("# of fields: {}", type_info->fields.size());
		gcc_jit_field *field = type_info->fields[load.idx];

		jit_instruction ji;
		ji.type_info = type_info->field_types[load.idx];
		ji.value = (gcc_jit_object *) gcc_jit_rvalue_access_field((gcc_jit_rvalue *) source.value, nullptr, field);

		return ji;
	}

	case Atom::type_index <Swizzle> ():
	{
		auto &swz = atom.as <Swizzle> ();

		// TODO: check that the field name is correct...

		jit_instruction source = context.cached[swz.src];

		// NOTE: assuming that the code -> field index is valid
		jit_struct *type_info = source.type_info;
		gcc_jit_field *field = type_info->fields[swz.code];

		jit_instruction ji;
		ji.type_info = type_info->field_types[swz.code];
		ji.value = (gcc_jit_object *) gcc_jit_rvalue_access_field((gcc_jit_rvalue *) source.value, nullptr, field);

		return ji;
	}

	// TODO: for store instructions, need to get a chain of lvalues...

	case Atom::type_index <List> ():
		// Skip lists, they are only for the Thunder IR
		return { nullptr, nullptr };

	case Atom::type_index <Returns> ():
	{
		auto &returns = atom.as <Returns> ();
		auto args = load_rvalue_arguments(context, returns.args);
		assert(args.size() == 1);

		gcc_jit_block_end_with_return(context.block, nullptr, args[0]);
	} return { nullptr, nullptr };

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

void generate_block(gcc_jit_context *const gcc, const Linkage::block_t &block)
{
	fmt::println("generating block");

	jit_context context {
		.gcc = gcc,
		.pool = block.unit,
	};

	// JIT the parameters
	std::vector <gcc_jit_param *> gcc_parameters;

	gcc_parameters.resize(block.parameters.size());
	context.parameters.resize(block.parameters.size());

	// TODO: fill the cache...
	for (auto &[i, t] : block.parameters) {
		std::string name = fmt::format("_arg{}", i);

		jit_struct *type_info = generate_type_field(context, t);
		gcc_jit_param *parameter = gcc_jit_context_new_param(context.gcc, nullptr, type_info->type, name.c_str());

		jit_instruction ji;
		ji.type_info = type_info;
		ji.value = (gcc_jit_object *) parameter;

		gcc_parameters[i] = parameter;
		context.parameters[i] = ji;
	}

	// Begin the function context
	// TODO: embed the scope name in the block
	fmt::println("block returns value: {} (ret: {})", block.unit[block.returns], block.returns);
	gcc_jit_type *returns = generate_type_field(context, block.returns)->type;
	context.function = gcc_jit_context_new_function(context.gcc, nullptr,
		GCC_JIT_FUNCTION_EXPORTED, returns, "function",
		context.parameters.size(), gcc_parameters.data(), 0);

	// TODO: distinguish between function_t and block_t

	context.block = gcc_jit_function_new_block(context.function, nullptr);
	context.cached.resize(block.unit.size());

	for (index_t i = 0; i < block.unit.size(); i++)
		context.cached[i] = generate_instruction(context, i);
}

Linkage::jit_result_t Linkage::generate_jit_gcc()
{
	dump();

	fmt::println("compiling linkage unit with gcc jit");

	gcc_jit_context *context = gcc_jit_context_acquire();
	assert(context);

	gcc_jit_context_set_int_option(context, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 0);
	gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_TREE, true);
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_SUMMARY, true);
	gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, true);

	for (auto &[_, block] : blocks)
		generate_block(context, block);

	gcc_jit_result *result = gcc_jit_context_compile(context);
	assert(result);

	void *ftn = gcc_jit_result_get_code(result, "function");
	assert(ftn);

	return { ftn };
}

} // namespace jvl::thunder
