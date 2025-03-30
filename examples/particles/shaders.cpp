#include <common/io.hpp>

#include "common/cmaps.hpp"

#include "app.hpp"

// Shader kernels for the sphere rendering
$entrypoint(vertex)
{
	layout_in <vec3> position(0);

	layout_out <vec3> out_position(0);
	layout_out <f32> out_speed(1);
	layout_out <vec2> out_range(2);

	readonly <buffer <unsized_array <vec3>>> translations(0);
	readonly <buffer <unsized_array <vec3>>> velocities(1);
	
	push_constant <MVP> view_info;

	vec3 translate = translations[gl_InstanceIndex];
	vec3 velocity = velocities[gl_InstanceIndex];
	
	gl_Position = view_info.project_vertex(translate, position);

	out_position = position;
	out_speed = length(velocity);
	out_range = vec2(view_info.smin, view_info.smax);
};

$partial_entrypoint(fragment)(ColorMap cmap)
{
	layout_in <vec3> position(0);
	layout_in <f32> speed(1);
	layout_in <vec2> range(2);

	layout_out <vec4> fragment(0);

	f32 t = (speed - range.x) / (range.y - range.x);

	fragment = vec4(cmap(t), 1.0f);
};

// Compiling pipelines
void Application::configure_pipeline(ColorMap cmap)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto vs = vertex;
	auto fs = fragment(cmap);

	auto vs_spv = link(vs).generate(Target::spirv_binary_via_glsl, Stage::vertex);
	auto fs_spv = link(fs).generate(Target::spirv_binary_via_glsl, Stage::fragment);

	// TODO: automatic generation by observing used layouts
	auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
		.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
		.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

	raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
		(resources.device, resources.window, resources.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex)
		.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex)
		.cull_mode(vk::CullModeFlagBits::eNone);
}

// Debugging
void Application::shader_debug()
{
	static const std::filesystem::path local = root() / "output" / "particles";
	
	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);

	auto vs = vertex;
	auto fs = fragment(jet);

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