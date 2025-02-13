#pragma once

#include <ire/core.hpp>

using namespace jvl;
using namespace jvl::ire;

// View information
struct ViewInfo {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 v) const {
		return proj * (view * (model * vec4(v, 1)));
	}

	auto layout() const {
		return uniform_layout("ViewInfo",
			named_field(model),
			named_field(view),
			named_field(proj));
	}
};

// Actual shaders
extern Procedure <void> vertex;
extern Procedure <void> fragment;