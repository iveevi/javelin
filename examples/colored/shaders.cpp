#include <common/io.hpp>

#include "common/vertex_flags.hpp"

#include "app.hpp"

// Shader kernels
$entrypoint(vertex)
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
};

$partial_entrypoint(fragment)(const glm::vec3 &C)
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
};

// Pipeline compilation
void Application::compile_pipeline(const glm::vec3 &color)
{
	auto vs = vertex;
	auto fs = fragment(color);

	auto vs_spv = link(vs).generate(Target::spirv_binary_via_glsl, Stage::vertex);
	auto fs_spv = link(fs).generate(Target::spirv_binary_via_glsl, Stage::fragment);

	// TODO: automatic generation by observing used layouts
	auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
		.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
		.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

	auto [binding, attributes] = binding_and_attributes(VertexFlags::ePosition);

	raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
		(resources.device, resources.window, resources.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_binding(binding)
		.with_vertex_attributes(attributes)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
}

// Debugging
void Application::shader_debug()
{
	static const std::filesystem::path local = root() / "output" / "colored";
	
	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);
		
	auto vs = vertex;
	auto fs = fragment(glm::vec3(1, 0, 1));

	std::string vertex_shader = link(vs).generate_glsl();
	std::string fragment_shader = link(fs).generate_glsl();

	io::display_lines("VERTEX", vertex_shader);
	io::display_lines("FRAGMENT", fragment_shader);

	vs.graphviz(local / "vertex.dot");
	fs.graphviz(local / "fragment.dot");

	thunder::optimize(vs);
	thunder::optimize(fs);

	vs.graphviz(local / "vertex-optimized.dot");
	fs.graphviz(local / "fragment-optimized.dot");

	link(vs, fs).write_assembly(local / "shaders.jvl.asm");
}