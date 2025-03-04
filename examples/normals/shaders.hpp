#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

// Model-view-projection structure
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};

// Actual shaders
extern Procedure <void> vertex;
extern Procedure <void> fragment;

extern void shader_debug();