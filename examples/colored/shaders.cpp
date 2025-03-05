#include <common/io.hpp>

#include "shaders.hpp"

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

void fragment(glm::vec3 C)
{
	layout_in <vec3> position(0);

	// Resulting fragment color
	layout_out <vec4> fragment(0);
	
	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	
	vec3 L = normalize(vec3(1, 1, 1));

	f32 lighting = 0.5 * max(dot(N, L), 0.0f) + 0.5f;

	vec3 color = lighting * vec3(C.x, C.y, C.z);

	fragment = vec4(color, 1);
}

// Debugging
void shader_debug()
{
	static const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path() / ".." / "..";
	static const std::filesystem::path local = root / "output" / "colored";
	
	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);
		
	auto vs_callable = procedure("main") << vertex;
	auto fs_callable = procedure("main") << std::make_tuple(glm::vec3(1, 0, 1)) << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	io::display_lines("VERTEX", vertex_shader);
	io::display_lines("FRAGMENT", fragment_shader);

	vs_callable.graphviz(local / "vertex.dot");
	fs_callable.graphviz(local / "fragment.dot");

	thunder::optimize(vs_callable);
	thunder::optimize(fs_callable);

	vs_callable.graphviz(local / "vertex-optimized.dot");
	fs_callable.graphviz(local / "fragment-optimized.dot");

	link(vs_callable, fs_callable).write_assembly(local / "shaders.jvl.asm");
}