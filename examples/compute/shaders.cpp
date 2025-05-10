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

	readonly <buffer <unsized_array <vec3>>> positions(0);
	readonly <buffer <unsized_array <vec3>>> velocities(1);
	
	push_constant <MVP> view_info;

	vec3 translate = positions[gl_InstanceIndex];
	vec3 velocity = velocities[gl_InstanceIndex];
	
	gl_Position = view_info.project_vertex(translate, position);

	out_position = position;
	out_speed = length(velocity);
	out_range = vec2(view_info.smin, view_info.smax);
};

$partial_entrypoint(fragment, ColorMap cmap)
{
	layout_in <vec3> position(0);
	layout_in <f32> speed(1);
	layout_in <vec2> range(2);

	layout_out <vec4> fragment(0);

	f32 t = (speed - range.x) / (range.y - range.x);

	fragment = vec4(cmap(t), 1.0f);
};

$entrypoint(integrator)
{
	local_size group(32);

	push_constant <SimulationInfo> info;
	
	// TODO: soft_t for unsized_array <vec3>
	buffer <unsized_array <vec3>> positions(0);
	buffer <unsized_array <vec3>> velocities(1);

	u32 tid = gl_GlobalInvocationID.x;

	$if (tid >= info.count) {
		$return $void;
	};

	// TODO: disable reads for writeonly buffers...
	vec3 p = positions[tid];
	vec3 v = velocities[tid];

	vec3 d;
	f32 l;

	d = (p - info.O1);	
	l = max(length(d), 1.0f);
	v = v - info.dt * info.M * d / pow(l, 3.0f);
	
	d = (p - info.O2);	
	l = max(length(d), 1.0f);
	v = v - info.dt * info.M * d / pow(l, 3.0f);

	p += info.dt * v;

	velocities[tid] = v;
	positions[tid] = p;
};

// Pipeline compilation
void Application::configure_render_pipeline(ColorMap cmap)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto vs = vertex;
	auto fs = fragment(cmap);

	auto vs_spv = link(vs).generate(Target::spirv_binary_via_glsl, Stage::vertex);
	auto fs_spv = link(fs).generate(Target::spirv_binary_via_glsl, Stage::fragment);

	auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
		.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
		.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

	raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
		(resources.device, resources.window, resources.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t<MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t<u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t<MVP>))
		.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer,
				1, vk::ShaderStageFlagBits::eVertex)
		.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer,
				1, vk::ShaderStageFlagBits::eVertex)
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void Application::configure_compute_pipeline()
{
	auto cs = integrator;
	auto cs_spv = link(cs).generate(Target::spirv_binary_via_glsl);
	auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
		.code(cs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eCompute);

	compute = littlevk::PipelineAssembler <littlevk::PipelineType::eCompute>
		(resources.device, resources.dal)
		.with_shader_bundle(bundle)
		.with_push_constant <SimulationInfo> (vk::ShaderStageFlagBits::eCompute)
		.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer,
				1, vk::ShaderStageFlagBits::eCompute)
		.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer,
				1, vk::ShaderStageFlagBits::eCompute);
}

// Debugging
void Application::shader_debug()
{
	set_trace_destination(root() / ".javelin");
	
	auto vs = vertex;
	auto fs = fragment(jet);
	auto cs = integrator;

	trace_unit("compute_raster", Stage::vertex, vs);
	trace_unit("compute_raster", Stage::fragment, fs);
	trace_unit("compute_integrator", Stage::compute, cs);
}