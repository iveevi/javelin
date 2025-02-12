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

	// Object triangle index
	layout_out <uint32_t, flat> out_id(0);

	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);

	// Transfering triangle index
	out_id = u32(gl_VertexIndex / 3);
}

void fragment(float saturation, float lightness, int splits)
{
	// Triangle index
	layout_in <uint32_t, flat> id(0);

	// Resulting fragment color
	layout_out <vec4> fragment(0);

	auto palette = hsl_palette(saturation, lightness, splits);

	fragment = vec4(palette[id % u32(palette.length)], 1);
}