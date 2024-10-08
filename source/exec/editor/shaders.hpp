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
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
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
inline void vertex(ViewportMode mode)
{
	// Vertex inputs
	layout_in <vec3> position(0);
	layout_in <vec3> normal(1);
	layout_in <vec2> uv(2);
	
	// Projection informations
	push_constant <MVP> mvp;
	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;

	// Auxiliary vertex shader outputs depending on the mode
	switch (mode) {
	case ViewportMode::eNormal:
	{
		// Regurgitate vertex positions
		layout_out <vec3> out_position(0);

		out_position = position;
	} break;

	case ViewportMode::eTextureCoordinates:
	{
		// TODO: allow duplicate bindings...
		layout_out <vec2> out_uv(2);
		
		out_uv = uv;
	} break;

	case ViewportMode::eTriangles:
	{
		// Object vertex index
		layout_out <uint32_t, flat> out_id(0);

		out_id = u32(gl_VertexIndex / 3);
	} break;

	default:
		break;
	}
}

// Fragment shader(s)
inline void fragment(ViewportMode mode)
{
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	switch (mode) {
	
	case ViewportMode::eNormal:
	{
		// Position from vertex shader
		layout_in <vec3> position(0);

		vec3 dU = dFdxFine(position);
		vec3 dV = dFdyFine(position);
		vec3 N = normalize(cross(dV, dU));

		fragment = vec4(0.5f + 0.5f * N, 1.0f);
	} break;

	case ViewportMode::eTextureCoordinates:
	{
		layout_in <vec2> uv(2);

		fragment = vec4(uv.x, uv.y, 0.0, 1.0f);
	} break;

	case ViewportMode::eTriangles:
	{
		// Object vertex index
		layout_in <uint32_t, flat> id(0);
		
		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[id % palette.length], 1);
	} break;

	case ViewportMode::eObject:
	{
		push_constant <ObjectInfo> info(solid_size <MVP>);

		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[info.id % palette.length], 1);

		// Early return to avoid PC conflicts with other pipelines
		return highlight_fragment(fragment, info.highlight);
	}

	case ViewportMode::eDepth:
	{
		float near = 0.1f;
		float far = 10000.0f;
		
		f32 d = gl_FragCoord.z;
		f32 linearized = (near * far) / (far + d * (near - far));

		linearized = ire::log(linearized/250.0f + 1);

		fragment = vec4(vec3(linearized), 1);
	} break;

	default:
		// Undefined, so highlight purple
		fragment = vec4(1, 0, 1, 1);
		break;
	}

	// Highlighting the selected object
	push_constant <u32> highlight(sizeof(solid_t <MVP>));

	highlight_fragment(fragment, highlight);
}