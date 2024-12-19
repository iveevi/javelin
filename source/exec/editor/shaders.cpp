#include "shaders.hpp"

// Vertex shader(s)
void vertex(RenderMode mode)
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
	case RenderMode::eNormalSmooth:
	{
		// Regurgitate vertex positions
		layout_out <vec3> out_normal(0);

		out_normal = normal;
	} break;

	case RenderMode::eNormalGeometric:
	case RenderMode::eBackup:
	{
		// Regurgitate vertex positions
		layout_out <vec3> out_position(0);

		out_position = position;
	} break;

	case RenderMode::eTextureCoordinates:
	{
		// TODO: allow duplicate bindings...
		layout_out <vec2> out_uv(2);

		out_uv = uv;
	} break;

	case RenderMode::eTriangles:
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
void fragment(RenderMode mode)
{
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	switch (mode) {

	case RenderMode::eNormalSmooth:
	{
		layout_in <vec3> normal(0);

		fragment = vec4(0.5f + 0.5f * normal, 1.0f);
	} break;

	case RenderMode::eNormalGeometric:
	{
		// Triangle index
		layout_in <vec3> position(0);

		// Render the normal vectors
		vec3 dU = dFdxFine(position);
		vec3 dV = dFdyFine(position);
		vec3 N = normalize(cross(dV, dU));
		fragment = vec4(0.5f + 0.5f * N, 1.0f);
	} break;

	case RenderMode::eTextureCoordinates:
	{
		layout_in <vec2> uv(2);

		fragment = vec4(uv.x, uv.y, 0.0, 1.0f);
	} break;

	case RenderMode::eTriangles:
	{
		// Object vertex index
		layout_in <uint32_t, flat> id(0);

		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[id % palette.length], 1);
	} break;

	case RenderMode::eObject:
	{
		push_constant <ObjectInfo> info(solid_size <MVP>);

		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[info.id % palette.length], 1);

		// Early return to avoid PC conflicts with other pipelines
		return highlight_fragment(fragment, info.highlight);
	}

	case RenderMode::eDepth:
	{
		float near = 0.1f;
		float far = 10000.0f;

		f32 d = gl_FragCoord.z;
		f32 linearized = (near * far) / (far + d * (near - far));

		linearized = ire::log(linearized/250.0f + 1);

		fragment = vec4(vec3(linearized), 1);
	} break;

	case RenderMode::eBackup:
	{
		// Position from vertex shader
		layout_in <vec3> position(0);
		// TODO: casting
		f32 modded = mod(floor(position.x) + floor(position.y) + floor(position.z), 2.0f);
		vec3 color(modded);
		fragment = vec4(color, 1);
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
