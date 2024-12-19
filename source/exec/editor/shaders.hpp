#pragma once

#include <ire/core.hpp>
#include <core/color.hpp>

#include "viewport.hpp"

using namespace jvl;
using namespace jvl::ire;

// Function to generate an HSV color palette in shader code
template <thunder::index_t N>
array <vec3> hsv_palette(float saturation, float lightness)
{
	std::array <vec3, N> palette;

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		float3 hsv = float3(i * step, saturation, lightness);
		float3 rgb = core::hsl_to_rgb(hsv);
		palette[i] = vec3(rgb.x, rgb.y, rgb.z);
	}

	return palette;
}

// Model-view-projection structure
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		// fmt::println("view info project (this = {})", (void *) this);
		// fmt::println("  model = {}", (void *) &model);
		// fmt::println("  view = {}", (void *) &view);
		// fmt::println("  proj = {}", (void *) &proj);
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		// fmt::println("[view info layout]");
		// fmt::println("  model = {}", (void *) &model);
		// fmt::println("  view = {}", (void *) &view);
		// fmt::println("  proj = {}", (void *) &proj);
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj)
		);
	}
};

// Material information
struct UberMaterial {
	vec3 kd;
	vec3 ks;
	f32 roughness;

	auto layout() const {
		return uniform_layout(
			"Material",
			named_field(kd),
			named_field(ks),
			named_field(roughness));
	}
};

// Albedo + highlight flag
struct Albedo {
	vec3 color;
	u32 highlight;

	auto layout() const {
		return uniform_layout("Albedo",
			named_field(color),
			named_field(highlight));
	}
};

// Object ID + highlight flag
struct ObjectInfo {
	u32 id;
	u32 highlight;

	auto layout() const {
		return uniform_layout("Object",
			named_field(id),
			named_field(highlight));
	}
};

// Highlighting
inline void highlight_fragment(layout_out <vec4> &fragment, const u32 &highlight)
{
	cond(highlight == 1u);
		fragment = mix(fragment, vec4(1, 1, 0, 1), 0.8f);
	end();
}

// Vertex shader(s)
void vertex(RenderMode);

// Fragment shader(s)
void fragment(RenderMode);