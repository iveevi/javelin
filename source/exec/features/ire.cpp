#include <fmt/format.h>

#include "ire/aggregate.hpp"
#include "ire/core.hpp"
#include "ire/solidify.hpp"
#include "ire/uniform_layout.hpp"
#include "profiles/targets.hpp"
#include "thunder/linkage.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"
#include "logging.hpp"
#include "constants.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types

using namespace jvl;
using namespace jvl::ire;

auto ftn = callable_info("shuffle") >> [](ivec3 in, i32 iterations)
{
	i32 counter = 0;
	loop(counter < iterations);
	{
		in.x <<= in.z;
		in.x >>= in.y;
		in.x |= in.y ^ in.z;
		in.x &= in.y - in.z;
		counter += 1;
	}
	end();

	return in;
};

int main()
{
	// ftn.dump();
	thunder::opt_transform(ftn);
	ftn.dump();

	fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));
	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::glsl_450));
}