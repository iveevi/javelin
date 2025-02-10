#pragma once

#include <ire/core.hpp>

using namespace jvl;
using namespace jvl::ire;

// Data layouts
struct Payload {
	u32 pindex;
	u32 resolution;

	auto layout() const {
		return uniform_layout("payload",
			named_field(pindex),
			named_field(resolution));
	}
};

struct ViewInfo {
	mat4 model;
	mat4 view;
	mat4 proj;
	u32 resolution;

	vec4 project(vec3 v) const {
		return proj * (view * (model * vec4(v, 1)));
	}

	auto layout() const {
		return uniform_layout("config",
			named_field(model),
			named_field(view),
			named_field(proj),
			named_field(resolution));
	}
};

// Actual shaders
extern Procedure <void> task;