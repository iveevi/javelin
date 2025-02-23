#include "common/color.hpp"

#include "shaders.hpp"

// Palette generation from SL values
array <vec3> hsl_palette(float saturation, float lightness, int N)
{
	std::vector <vec3> palette(N);

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		glm::vec3 hsl = glm::vec3(i * step, saturation, lightness);
		glm::vec3 rgb = hsl_to_rgb(hsl);
		palette[i] = vec3(rgb.x, rgb.y, rgb.z);
	}

	return palette;
}

// Shader kernels
void vertex()
{
	// Vertex inputs
	layout_in <vec3> position(0);

	// Stage outputs
	layout_out <vec3> out_position(0);
	layout_out <u32, flat> out_id(1);

	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);

	out_position = position;
	out_id = u32(gl_VertexIndex / 3);
}

void fragment(float saturation, float lightness, int splits)
{
	layout_in <vec3> position(0);
	layout_in <u32, flat> id(1);

	// Resulting fragment color
	layout_out <vec4> fragment(0);
	
	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	
	vec3 L = normalize(vec3(1, 1, 1));

	f32 lighting = 0.5 * max(dot(N, L), 0.0f) + 0.5f;

	auto palette = hsl_palette(saturation, lightness, splits);

	vec3 color = lighting * palette[id % u32(palette.length)];

	fragment = vec4(color, 1);
}