#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

static constexpr uint32_t MESHLET_SIZE = 32;

// View information
struct ViewInfo {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 v) const {
		return proj * (view * (model * vec4(v, 1)));
	}

	auto layout() {
		return layout_from("ViewInfo",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};

// Task payload information
struct Payload {
	u32 pid;
	u32 offset;
	u32 size;

	auto layout() {
		return layout_from("Payload",
			verbatim_field(pid),
			verbatim_field(offset),
			verbatim_field(size));
	}
};

// Actual shaders
extern Procedure <void> task;
extern Procedure <void> mesh;
extern Procedure <void> fragment;

extern void shader_debug();