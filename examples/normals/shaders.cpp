#include <common/io.hpp>

#include "shaders.hpp"

// Shader kernels
$entrypoint(vertex)
{
	// Vertex inputs
	layout_in <vec3> position(0);
	
	// Stage outputs
	layout_out <vec3> out_position(0);
	
	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);

	// Regurgitate positions
	out_position = position;
};

$entrypoint(fragment)
{
	layout_in <vec3> position(0);
	
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	// Render the normal vectors
	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
};

// Debugging
void shader_debug()
{
	static const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path() / ".." / "..";

	set_trace_destination(root / ".javelin");

	trace_unit("normals", Stage::vertex, vertex);
	trace_unit("normals", Stage::vertex, fragment);
}