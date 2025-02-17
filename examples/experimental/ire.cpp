// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire.hpp>
#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

#include "common/util.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

struct RayFrame {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return layout_from("RayFrame",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical));
	}
};

auto rgen = procedure <void> ("main") << []()
{
	push_constant <RayFrame> rayframe;

	accelerationStructureEXT tlas(0);

	ray_payload <vec3> payload(0);

	// TODO: image formats restricted by native type
	write_only <image2D> image(1);

	// TODO: simpler vector constructors...
	vec2 center = vec2(gl_LaunchIDEXT.xy()) + vec2(0.5);
	vec2 uv = center / vec2(imageSize(image));
	vec3 ray = rayframe.at(uv);

	// vec4 color = vec4(pow(payload, vec3(1/2.2)), 1.0);
	vec4 color = gl_LaunchIDEXT.yyxz();

	imageStore(image, ivec2(gl_LaunchIDEXT.xy()), color);
};

int main()
{
	rgen.dump();
	dump_lines(link(rgen).generate_glsl());
	link(rgen).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
