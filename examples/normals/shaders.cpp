#include <common/io.hpp>

#include "shaders.hpp"

// Shader kernels
Procedure <void> vertex = procedure("main") << []()
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

Procedure <void> fragment = procedure("main") << []()
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
	static const std::filesystem::path local = root / "output" / "normals";
	
	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);

	std::string vertex_shader = link(vertex).generate_glsl();
	std::string fragment_shader = link(fragment).generate_glsl();

	io::display_lines("VERTEX", vertex_shader);
	io::display_lines("FRAGMENT", fragment_shader);

	vertex.graphviz(local / "vertex.dot");
	fragment.graphviz(local / "fragment.dot");

	thunder::optimize(vertex);
	thunder::optimize(fragment);

	vertex.graphviz(local / "vertex-optimized.dot");
	fragment.graphviz(local / "fragment-optimized.dot");

	link(vertex, fragment).write_assembly(local / "shaders.jvl.asm");
}