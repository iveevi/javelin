// TODO: autodiff on inputs, for callables and shaders
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

auto ftn = procedure <void> ("main") << []()
{
	write_only <buffer <unsized_array <vec3>>> bf(0);
	bf[1].x = 45;
};

// TODO: shadertoy example

int main()
{
	ftn.dump();
	dump_lines("EXPERIMENTAL IRE", link(ftn).generate_glsl());
	link(ftn).generate_spirv(vk::ShaderStageFlagBits::eRaygenKHR);
}
