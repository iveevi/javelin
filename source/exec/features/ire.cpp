// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

auto fragment = callable("main")
	<< std::make_tuple(true)
	<< [](bool texture)
{
	layout_in <vec2> uv(2);

	layout_out <vec4> fragment(0);

	if (texture) {
		sampler2D albedo(0);

		fragment = albedo.sample(uv);
		cond(fragment.w < 0.1f);
			discard();
		end();
	}
};

int main()
{
	// thunder::opt_transform(fragment);
	fragment.dump();
	auto unit = link(fragment);
	fmt::println("{}", unit.generate_glsl());
}