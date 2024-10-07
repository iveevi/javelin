#pragma once

#include <ire/core.hpp>
#include <core/color.hpp>

#include "viewport_render_group.hpp"

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
		return uniform_layout("MVP",
			named_field(color),
			named_field(highlight));
	}
};

// Vertex shader(s)
inline void vertex(ViewportMode mode)
{
	// Vertex inputs
	layout_in <vec3> position(0);
	layout_in <vec3> normal(1);
	layout_in <vec2> uv(2);

	// Regurgitate vertex positions
	layout_out <vec3> out_position(0);

	// TODO: allow duplicate bindings...
	layout_out <vec2> out_uv(2);

	// Object vertex ID
	layout_out <uint32_t, flat> out_id(0);
	
	// Projection informations
	push_constant <MVP> mvp;
	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;

	switch (mode) {
	case ViewportMode::eNormal:
		out_position = position;
		break;
	case ViewportMode::eTextureCoordinates:
		out_uv = uv;
		break;
	case ViewportMode::eTriangles:
		out_id = u32(gl_VertexIndex / 3);
		break;
	default:
		break;
	}
}

// Fragment shader(s)
inline void fragment(ViewportMode mode)
{
	// Position from vertex shader
	layout_in <vec3> position(0);
	layout_in <vec2> uv(2);

	// Object vertex ID
	layout_in <uint32_t, flat> id(0);

	// Resulting fragment color
	layout_out <vec4> fragment(0);

	switch (mode) {
	
	case ViewportMode::eNormal:
	{
		vec3 dU = dFdxFine(position);
		vec3 dV = dFdyFine(position);
		vec3 N = normalize(cross(dV, dU));
		fragment = vec4(0.5f + 0.5f * N, 1.0f);
	} break;

	case ViewportMode::eTextureCoordinates:
		fragment = vec4(uv.x, uv.y, 0.0, 1.0f);
		break;

	case ViewportMode::eTriangles:
	{
		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[id % palette.length], 1);
	} break;

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
		fragment = vec4(1, 0, 1, 1);
		break;
	}

	// Highlighting the selected object
	push_constant <u32> highlight(sizeof(solid_t <MVP>));

	cond(highlight == 1u);
		fragment = mix(fragment, vec4(1, 1, 0, 1), 0.8f);
	end();
}