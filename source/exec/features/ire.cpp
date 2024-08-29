#include <concepts>
#include <deque>

#include <fmt/format.h>
#include <functional>

#include "ire/core.hpp"
#include "ire/ad/core.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

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
	return sin(x * x);
	// return (x / y) + x;
}

auto id = callable(__id).named("id");

int main()
{
	id.dump();
	auto did = dfwd(id);
	did.dump();

	auto shader = [&]() {
		layout_in <float> input(0);
		layout_out <float> output(0);

		dual_t <f32> dx = dual(id(input, 1), f32(1.0f));
		dual_t <f32> dy = dual(id(1, (f32) input / 2.0f), input);

		output = did(dx, dy).dual;
	};

	auto kernel = kernel_from_args(shader);

	// kernel.dump();

	fmt::println("{}", kernel.synthesize(profiles::opengl_450));
}
