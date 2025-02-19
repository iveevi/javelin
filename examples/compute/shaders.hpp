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
		return layout_from("ViewInfo",
			verbatim_field(view),
			verbatim_field(proj),
			verbatim_field(smin),
			verbatim_field(smax));
	}
};

// Simulation meta data
struct SimulationInfo {
	vec3 O1;
	vec3 O2;
	f32 M;
	f32 dt;
	u32 count;

	auto layout() {
		return layout_from("SimulationInfo",
			verbatim_field(O1),
			verbatim_field(O2),
			verbatim_field(M),
			verbatim_field(dt),
			verbatim_field(count));
	}
};

// Actual shaders
extern void vertex();
extern void fragment(vec3 (*)(f32));
extern void integrator();