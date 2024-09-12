#include "ire/core.hpp"
#include "profiles/targets.hpp"
#include "thunder/opt.hpp"

// TODO: immutability for shader inputs types
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types
// TODO: binary operations
// TODO: external constant specialization

using namespace jvl;
using namespace jvl::ire;

struct shuffle_info {
	i32 iterations;
	i32 stride;

	auto layout() const {
		return uniform_layout("shuffle_info",
			named_field(iterations),
			named_field(stride));
	}
};

auto ftn = callable_info("shuffle") >> [](ivec3 in, shuffle_info info)
{
	// TODO: color wheel generator
	array <i32, 3> list { 1, 2, 3 };
	list[1] = in.x + in.y / in.z;
	i32 x = in.y - in.z/in.x;
	x += 4 * in.z;
	return list[2] * list[1] / x;
};

int main()
{
	thunder::opt_transform(ftn);
	ftn.dump();

	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));
	fmt::println("{}", ftn.export_to_kernel().compile(profiles::glsl_450));
}