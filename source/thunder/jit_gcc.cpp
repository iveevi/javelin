#include <libgccjit.h>

#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"

namespace jvl::thunder {

struct jit_struct {
	// TODO: header with eightcc and hash for cleanup
	gcc_jit_type *type;
	std::vector <gcc_jit_field *> fields;
	std::vector <jit_struct *> field_types;
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

jit_struct *generate_type_field_primitive_vector(jit_context &context, const char *name, PrimitiveType item, size_t components)
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

	// TODO: materialize() method
	s->type = (gcc_jit_type *) gcc_jit_context_new_struct_type(context.gcc, nullptr,
		name, s->fields.size(), s->fields.data());

	return s;
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
	if (tf.next != -1 || tf.down != -1) {
		fmt::println("GCC JIT version does not supported aggregate type");
		abort();
	}

	return generate_type_field_primitive(context, tf.item);
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

jit_instruction generate_instruction(jit_context &context, index_t i)
{
	auto &atom = context.pool[i];

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
		jit_instruction ji;
		ji.value = nullptr;
		ji.type_info = generate_type_field(context, i);
		return ji;
	}

	case Atom::type_index <Primitive> ():
	{
		auto &primitive = atom.as <Primitive> ();

		jit_instruction ji;

		switch (primitive.type) {

		case i32:
		{
			ji.type_info = generate_type_field_primitive(context, i32);
			ji.value = (gcc_jit_object *) gcc_jit_context_new_rvalue_from_int(context.gcc,
					ji.type_info->type, primitive.idata);
			return ji;
		}

		case f32:
		{
			ji.type_info = generate_type_field_primitive(context, f32);
			ji.value = (gcc_jit_object *) gcc_jit_context_new_rvalue_from_double(context.gcc,
					ji.type_info->type, primitive.fdata);
			return ji;
		}

		default:
		{
			fmt::println("unexpected primitive type: {}", primitive);
			abort();
		}

		}
	}

	case Atom::type_index <Construct> ():
	{
		auto &ctor = atom.as <Construct> ();
		auto args = load_rvalue_arguments(context, ctor.args);

		jit_struct *s = context.cached[ctor.type].type_info;

		// TODO: construct() method
		gcc_jit_rvalue *value = gcc_jit_context_new_struct_constructor(context.gcc,
			nullptr, s->type, args.size(), s->fields.data(), args.data());

		jit_instruction ji;
		ji.type_info = s;
		ji.value = (gcc_jit_object *) value;

		return ji;
	}

	case Atom::type_index <Operation> ():
	{
		auto &opn = atom.as <Operation> ();
		auto args = load_rvalue_arguments(context, opn.args);

		jit_struct *returns = context.cached[opn.type].type_info;

		gcc_jit_rvalue *expr;
		switch (opn.code) {

		case addition:
		{
			expr = gcc_jit_context_new_binary_op(context.gcc, nullptr,
				GCC_JIT_BINARY_OP_PLUS,
				returns->type, args[0], args[1]);
		} break;

		case subtraction:
		{
			expr = gcc_jit_context_new_binary_op(context.gcc, nullptr,
				GCC_JIT_BINARY_OP_MINUS,
				returns->type, args[0], args[1]);
		} break;

		case multiplication:
		{
			expr = gcc_jit_context_new_binary_op(context.gcc, nullptr,
				GCC_JIT_BINARY_OP_MULT,
				returns->type, args[0], args[1]);
		} break;

		case division:
		{
			expr = gcc_jit_context_new_binary_op(context.gcc, nullptr,
				GCC_JIT_BINARY_OP_DIVIDE,
				returns->type, args[0], args[1]);
		} break;

		default:
		{
			fmt::println("GCC JIT unsupported operation {}",
				tbl_operation_code[opn.code]);
			abort();
		}

		}

		fmt::println("result of operation is {} -> {}", (void *) expr, i);

		jit_instruction ji;
		ji.type_info = returns;
		ji.value = (gcc_jit_object *) expr;

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
		assert(args.size() == 1);

		// Resulting value
		gcc_jit_rvalue *value = nullptr;

		switch (intr.opn) {

		case sin:
		{
			gcc_jit_function *intr_ftn = gcc_jit_context_get_builtin_function(context.gcc, "sin");
			assert(intr_ftn);

			// Casting the arguments
			gcc_jit_type *double_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_DOUBLE);
			gcc_jit_type *float_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_FLOAT);

			assert(double_type);
			args[0] = gcc_jit_context_new_cast(context.gcc, nullptr, args[0], double_type);
			assert(args[0]);
			value = gcc_jit_context_new_call(context.gcc, nullptr, intr_ftn, 1, args.data());
			value = gcc_jit_context_new_cast(context.gcc, nullptr, value, float_type);
		} break;

		case cos:
		{
			gcc_jit_function *intr_ftn = gcc_jit_context_get_builtin_function(context.gcc, "cos");
			assert(intr_ftn);

			// Casting the arguments
			gcc_jit_type *double_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_DOUBLE);
			gcc_jit_type *float_type = gcc_jit_context_get_type(context.gcc, GCC_JIT_TYPE_FLOAT);

			assert(double_type);
			args[0] = gcc_jit_context_new_cast(context.gcc, nullptr, args[0], double_type);
			assert(args[0]);
			value = gcc_jit_context_new_call(context.gcc, nullptr, intr_ftn, 1, args.data());
			value = gcc_jit_context_new_cast(context.gcc, nullptr, value, float_type);
		} break;

		default:
		{
			fmt::println("GCC JIT unsupported intrinsic <{}>",
				tbl_intrinsic_operation[intr.opn]);
			abort();
		}

		}

		jit_instruction ji;
		ji.type_info = context.cached[intr.type].type_info;
		ji.value = (gcc_jit_object *) value;

		return ji;
	}

	default:
	{
		fmt::println("GCC JIT unsupported instruction: {}", atom.to_string());
		abort();
	}

	}

	return { nullptr, nullptr };
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
	// gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, true);

	for (auto &[_, block] : blocks)
		generate_block(context, block);

	gcc_jit_result *result = gcc_jit_context_compile(context);
	assert(result);

	void *ftn = gcc_jit_result_get_code(result, "function");
	assert(ftn);

	return { ftn };
}

} // namespace jvl::thunder
