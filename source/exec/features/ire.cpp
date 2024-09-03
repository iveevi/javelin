#include <fmt/format.h>

#include <libgccjit.h>
#include <type_traits>
#include <vector>

#include "ire/core.hpp"
#include "ire/uniform_layout.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types

using namespace jvl;
using namespace jvl::ire;

// Sandbox application
f32 __ftn(f32 x, f32 y)
{
	return cos(x) * y;
	// return (x / y) + x;
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
	auto load_args = [&](index_t argsi) {
		std::vector <gcc_jit_rvalue *> args;

		index_t next = argsi;
		while (next != -1) {
			auto &g = block.unit[next];
			assert(g.is <List> ());

			auto &list = g.as <List> ();
			args.push_back((gcc_jit_rvalue *) cached[list.item]);

			next = list.next;
		}

		return args;
	};

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
			auto args = load_args(opn.args);

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
			
			case multiplication:
			{
				expr = gcc_jit_context_new_binary_op(context, nullptr,
					GCC_JIT_BINARY_OP_MULT,
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
			auto args = load_args(returns.args);
			assert(args.size() == 1);

			gcc_jit_block_end_with_return(primary, nullptr, args[0]);
		} break;

		case Atom::type_index <Intrinsic> ():
		{
			auto &intr = atom.as <Intrinsic> ();
			auto args = load_args(intr.args);
			assert(args.size() == 1);

			// Resulting value
			gcc_jit_rvalue *value = nullptr;

			switch (intr.opn) {

			case cos:
			{
				gcc_jit_function *intr_ftn = gcc_jit_context_get_builtin_function(context, "cos");
				assert(intr_ftn);

				// Casting the arguments
				gcc_jit_type *double_type = gcc_jit_context_get_type(context, GCC_JIT_TYPE_DOUBLE);
				gcc_jit_type *float_type = gcc_jit_context_get_type(context, GCC_JIT_TYPE_FLOAT);

				assert(double_type);
				args[0] = gcc_jit_context_new_cast(context, nullptr, args[0], double_type);
				assert(args[0]);
				value = gcc_jit_context_new_call(context, nullptr, intr_ftn, 1, args.data());
				value = gcc_jit_context_new_cast(context, nullptr, value, float_type);
			} break;
			
			default:
			{
				fmt::println("GCC JIT unsupported intrinsic <{}>",
					tbl_intrinsic_operation[intr.opn]);
				abort();
			}

			}
			
			cached[i] = value;
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

template <typename R, typename ... Args>
auto jit(const callable_t <R, Args...> &callable)
{
	using function_t = solid_t <R> (*)(solid_t <Args> ...);
	auto kernel = callable.export_to_kernel();
	auto linkage = kernel.linkage().resolve();
	auto jr = linkage.generate_jit_gcc();
	return function_t(jr.result);
}

struct lighting {
	vec3 direction;
	vec3 color;

	auto layout() const {
		return uniform_layout(direction, color);
	}
};

struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;

	lighting light;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(model, view, proj, light);
	}
};

tuple_index <0> m_model;
tuple_index <1> m_view;
tuple_index <2> m_proj;
tuple_index <3> m_light;

tuple_index <0> m_direction;
tuple_index <1> m_color;

auto t = solid_t <mvp> ();
float x = t[m_light][m_direction].x;

static_assert(std::same_as <std::decay_t <decltype(t[m_model])>, float4x4>);
static_assert(std::same_as <std::decay_t <decltype(t[m_view])>, float4x4>);
static_assert(std::same_as <std::decay_t <decltype(t[m_proj])>, float4x4>);
// static_assert(std::same_as <std::decay_t <decltype(t[m_light][m_direction])>, float3>);
// static_assert(std::same_as <std::decay_t <decltype(t[m_light][m_color])>, float3>);

static_assert(sizeof(solid_t <mvp>) == 3 * sizeof(float4x4) + 2 * sizeof(float4));

int main()
{
	thunder::opt_transform_compact(id);
	thunder::opt_transform_dead_code_elimination(id);
	id.dump();

	auto jit_ftn = jit(id);
	fmt::println("result: {}", jit_ftn(1, 1));
}