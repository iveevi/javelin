#pragma once

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

struct Constants {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;
	vec3 light;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return layout_from("Constants",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical),
			verbatim_field(light));
	}
};

struct Reference {
	u64 vertices;
	u64 triangles;

	auto layout() {
		return layout_from("Reference",
			verbatim_field(vertices),
			verbatim_field(triangles));
	}
};

// Actual shaders
extern Procedure <void> quad;
extern Procedure <void> blit;

extern Procedure <void> ray_generation;
extern Procedure <void> primary_closest_hit;
extern Procedure <void> primary_miss;
extern Procedure <void> shadow_miss;

extern void shader_debug();