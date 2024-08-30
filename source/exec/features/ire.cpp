#include <concepts>
#include <deque>

#include <fmt/format.h>
#include <functional>

#include "ire/core.hpp"
#include "ire/ad/core.hpp"
#include "ire/emitter.hpp"
#include "ire/scratch.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/opt.hpp"
#include "thunder/enumerations.hpp"
#include "wrapped_types.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: pass structs as inout to callables
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again

using namespace jvl;
using namespace jvl::ire;

// Sandbox application
f32 __id(f32 x, f32 y)
{
	// return sin(x * x);
	return (x / y) + x;
}

auto id = callable(__id).named("id");

int main()
{
	auto did = dfwd(id);
	
	Callable optimized = did;

	optimized.dump();

	thunder::opt_transform_compact(optimized);
	thunder::opt_transform_constructor_elision(optimized);
	thunder::opt_transform_dead_code_elimination(optimized);

	optimized.dump();

	Callable original = did;

	fmt::println("{}", original.export_to_kernel().synthesize(profiles::opengl_450));
	fmt::println("{}", optimized.export_to_kernel().synthesize(profiles::opengl_450));
}
