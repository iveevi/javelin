#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

// Data layouts
struct Payload {
	u32 pindex;
	u32 resolution;

	auto layout() {
		return layout_from("Payload",
			verbatim_field(pindex),
			verbatim_field(resolution));
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

	auto layout() {
		return layout_from("ViewInfo",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj),
			verbatim_field(resolution));
	}
};

// Actual shaders
extern Procedure <void> task;
extern Procedure <void> mesh;
extern Procedure <void> fragment;

extern void shader_debug();