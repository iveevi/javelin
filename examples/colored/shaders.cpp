#include "shaders.hpp"

// Shader kernels
void vertex()
{
	// Vertex inputs
	layout_in <vec3> position(0);
	
	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);
}

void fragment(glm::vec3 color)
{
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	fragment = vec4(color.x, color.y, color.z, 1);
}