#pragma once

#include <ire/core.hpp>

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

	auto layout() const {
		return uniform_layout(
			"ViewInfo",
			named_field(view),
			named_field(proj),
			named_field(smin),
			named_field(smax));
	}
};

// Simulation meta data
struct SimulationInfo {
	vec3 O1;
	vec3 O2;
	f32 M;
	f32 dt;

	auto layout() const {
		return uniform_layout("SimulationInfo",
			named_field(O1),
			named_field(O2),
			named_field(M),
			named_field(dt));
	}
};

// Actual shaders
extern void vertex();
extern void fragment(vec3 (*)(f32));
extern void integrator();