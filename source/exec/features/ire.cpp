#include <fmt/format.h>

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
f32 __ftn(vec2 v)
{
	// return cos(v);
	return cos(v.x);
	// return sin(cos(v.x) - v.y);
}

auto id = callable(__ftn).named("ftn");

template <typename R, typename ... Args>
auto jit(const callable_t <R, Args...> &callable)
{
	using function_t = solid_t <R> (*)(solid_t <Args> ...);
	auto kernel = callable.export_to_kernel();
	auto linkage = kernel.linkage().resolve();
	auto jr = linkage.generate_jit_gcc();
	return function_t(jr.result);
}

int main()
{
	thunder::opt_transform_compact(id);
	thunder::opt_transform_dead_code_elimination(id);
	id.dump();

	auto jit_ftn = jit(id);
}