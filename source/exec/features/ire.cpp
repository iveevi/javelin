#include <fmt/format.h>

#include <libgccjit.h>

#include "ire/core.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again

using namespace jvl;
using namespace jvl::ire;

// Sandbox application
f32 __ftn(f32 x, f32 y)
{
	return (x / y) + x;
}

auto id = callable(__ftn).named("ftn");

namespace jvl::thunder {

gcc_jit_type *libgccjit_type_field(gcc_jit_context *const context, const Linkage::source_block_t &pool, index_t i)
{
	auto &atom = pool[i];
	assert(atom.is <TypeField> ());
	
	TypeField tf = atom.as <TypeField> ();
	if (tf.next != -1 || tf.down != -1) {
		fmt::println("GCC JIT version does not supported aggregate type");
		abort();
	}

	switch (tf.item) {
	case i32: return gcc_jit_context_get_type(context, GCC_JIT_TYPE_INT);
	case f32: return gcc_jit_context_get_type(context, GCC_JIT_TYPE_FLOAT);
	default:
	{
		fmt::println("GCC JIT unsupported type {}",
			tbl_primitive_types[tf.item]);
		abort();
	}
	}

	return nullptr;
}

void libgccjit_block(gcc_jit_context *const context, const Linkage::block_t &block)
{
	fmt::println("generating block");

	// JIT the parameters
	std::vector <gcc_jit_param *> parameters(block.parameters.size());

	for (auto &[i, t] : block.parameters) {
		std::string name = fmt::format("_arg{}", i);
		gcc_jit_type *type = libgccjit_type_field(context, block.unit, t);
		parameters[i] = gcc_jit_context_new_param(context, nullptr, type, name.c_str());
	}

	// Begin the function context
	// TODO: embed the scope name in the block
	gcc_jit_type *returns = libgccjit_type_field(context, block.unit, block.returns);
	gcc_jit_function *function = gcc_jit_context_new_function(context, nullptr,
		GCC_JIT_FUNCTION_EXPORTED, returns, "function",
		parameters.size(), parameters.data(), 0);

	// TODO: distinguish between function_t and block_t

	gcc_jit_block *primary = gcc_jit_function_new_block(function, nullptr);

	std::vector <void *> cached(block.unit.size());
	for (index_t i = 0; i < block.unit.size(); i++) {
		auto &atom = block.unit[i];

		switch (atom.index()) {

		case Atom::type_index <Global> ():
		{
			auto &global = atom.as <Global> ();
			assert(global.qualifier == parameter);
			gcc_jit_param *p = parameters[global.binding];
			cached[i] = gcc_jit_param_as_rvalue(p);
		} break;

		case Atom::type_index <TypeField> ():
		{
			cached[i] = libgccjit_type_field(context, block.unit, i);
		} break;

		case Atom::type_index <Operation> ():
		{
			auto &opn = atom.as <Operation> ();

			std::vector <gcc_jit_rvalue *> args;

			index_t next = opn.args;
			while (next != -1) {
				auto &g = block.unit[next];
				assert(g.is <List> ());

				auto &list = g.as <List> ();
				args.push_back((gcc_jit_rvalue *) cached[list.item]);

				next = list.next;
			}

			fmt::println("args: {}", args.size());
			for (auto arg : args)
				fmt::println("  {}", (void *) arg);

			gcc_jit_type *returns = (gcc_jit_type *) cached[opn.type];

			gcc_jit_rvalue *expr;
			switch (opn.code) {
			
			case addition:
			{
				expr = gcc_jit_context_new_binary_op(context, nullptr,
					GCC_JIT_BINARY_OP_PLUS,
					returns, args[0], args[1]);
			} break;
			
			case division:
			{
				expr = gcc_jit_context_new_binary_op(context, nullptr,
					GCC_JIT_BINARY_OP_DIVIDE,
					returns, args[0], args[1]);
			} break;

			default:
			{
				fmt::println("GCC JIT unsupported operation {}",
					tbl_operation_code[opn.code]);
				abort();
			}

			}

			fmt::println("result of operation is {} -> {}", (void *) expr, i);

			cached[i] = expr;
		} break;

		case Atom::type_index <List> ():
		{
			// Skip lists, they are only for the Thunder IR
		} break;

		case Atom::type_index <Returns> ():
		{
			auto &returns = atom.as <Returns> ();

			std::vector <gcc_jit_rvalue *> args;
			index_t next = returns.args;
			while (next != -1) {
				auto &g = block.unit[next];
				assert(g.is <List> ());

				auto &list = g.as <List> ();
				args.push_back((gcc_jit_rvalue *) cached[list.item]);

				next = list.next;
			}

			assert(args.size() == 1);

			gcc_jit_block_end_with_return(primary, nullptr, args[0]);
		} break;

		default:
		{
			fmt::println("GCC JIT unsupported instruction:");
			dump_ir_operation(atom);
			fmt::print("\n");
			abort();
		}

		}
	}
}

Linkage::jit_result_t Linkage::generate_jit_gcc()
{
	fmt::println("compiling linkage unit with gcc jit");

	gcc_jit_context *context = gcc_jit_context_acquire();
	assert(context);
	
	gcc_jit_context_set_bool_option(context,
		GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, 1);

	for (auto &[_, block] : blocks)
		libgccjit_block(context, block);
	
	gcc_jit_result *result = gcc_jit_context_compile(context);
	assert(result);

	void *ftn = gcc_jit_result_get_code(result, "function");
	assert(ftn);

	return { ftn };
}

}

int main()
{
	thunder::opt_transform_compact(id);
	thunder::opt_transform_dead_code_elimination(id);
	id.dump();

	auto kernel = id.export_to_kernel();
	auto linkage = kernel.linkage().resolve();

	auto jit = linkage.generate_jit_gcc();

	using ftn_t = float (*)(float, float);
	auto jit_ftn = (ftn_t) jit.result;
	fmt::println("result: {}", jit_ftn(1, 1));
}