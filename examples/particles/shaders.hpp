#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

// View and projection information
struct MVP {
	mat4 view;
	mat4 proj;
	f32 smin;
	f32 smax;

	vec4 project_vertex(const vec3 &position, const vec3 &vertex) {
		vec4 p = vec4(vertex + position, 1);
		return proj * (view * p);
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(view),
			verbatim_field(proj),
			verbatim_field(smin),
			verbatim_field(smax));
	}
};

// Actual shaders
extern void vertex();
extern void fragment(vec3 (*)(f32));

extern void shader_debug();