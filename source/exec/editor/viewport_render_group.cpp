#include "viewport_render_group.hpp"
#include "viewport.hpp"

// HSV palette generation
// TODO: hsv util
vec3 hsv_to_rgb(const float3 &hsv)
{
	float h = hsv.x;
	float s = hsv.y;
	float v = hsv.z;

	float c = v * s;
	float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
	float m = v - c;

	float r;
	float g;
	float b;
	if (0 <= h && h < 60) {
		r = c;
		g = x;
		b = 0;
	} else if (60 <= h && h < 120) {
		r = x;
		g = c;
		b = 0;
	} else if (120 <= h && h < 180) {
		r = 0;
		g = c;
		b = x;
	} else if (180 <= h && h < 240) {
		r = 0;
		g = x;
		b = c;
	} else if (240 <= h && h < 300) {
		r = x;
		g = 0;
		b = c;
	} else {
		r = c;
		g = 0;
		b = x;
	}

	return vec3((r + m), (g + m), (b + m));
}

// Function to generate an HSV color palette
template <thunder::index_t N>
array <vec3, N> hsv_palette(float saturation, float lightness)
{
	std::array <vec3, N> palette;

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		float3 hsv = float3(i * step, saturation, lightness);
		vec3 rgb = hsv_to_rgb(hsv);
		palette[i] = rgb;
	}

	return palette;
}

// Shader sources
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;
	i32 id;
	i32 selected;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj),
			named_field(id),
			named_field(selected)
		);
	}
};

void vertex(ViewportMode mode)
{
	layout_in <vec3> position(0);
	layout_out <vec3> out_position(0);
	layout_out <int, flat> out_id(0);

	layout_out <int, flat> out_uuid(1);
	layout_out <int, flat> out_selected(2);
	
	push_constant <MVP> mvp;
	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;

	switch (mode) {
	case eNormal:
		out_position = position;
		break;
	case eTriangles:
		out_id = (gl_VertexIndex / 3);
		break;
	default:
		break;
	}

	out_uuid = mvp.id;
	out_selected = mvp.selected;	
}

void fragment(ViewportMode mode)
{
	layout_in <vec3> position(0);
	layout_in <int, flat> id(0);
	layout_out <vec4> fragment(0);
	
	layout_in <int, flat> uuid(1);
	layout_in <int, flat> selected(2);

	switch (mode) {
	
	case eNormal:
	{
		vec3 dU = dFdxFine(position);
		vec3 dV = dFdyFine(position);
		vec3 N = normalize(cross(dV, dU));
		fragment = vec4(0.5f + 0.5f * N, 1.0f);
	} break;

	case eTriangles:
	{
		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[id % palette.size()], 1);
	} break;

	case eDepth:
	{
		float near = 0.1f;
		float far = 10000.0f;
		
		f32 d = gl_FragCoord.z;
		f32 linearized = (near * far) / (far + d * (near - far));

		linearized = ire::log(linearized/250.0f + 1);

		fragment = vec4(vec3(linearized), 1);
	} break;

	default:
		fragment = vec4(1, 0, 1, 1);
		break;
	}

	// Highlighting the selected object
	cond(uuid == selected);
		fragment = mix(fragment, vec4(1, 1, 0, 1), 0.8f);
	end();
}

// Constructor	
ViewportRenderGroup::ViewportRenderGroup(DeviceResourceCollection &drc)
{
	configure_render_pass(drc);
	configure_pipelines(drc);
}

// Configuration
void ViewportRenderGroup::configure_render_pass(DeviceResourceCollection &drc)
{
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
	.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
	.add_attachment(littlevk::default_depth_attachment())
	.add_subpass(vk::PipelineBindPoint::eGraphics)
		.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
		.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.done();
}

void ViewportRenderGroup::configure_pipeline_mode(DeviceResourceCollection &drc, ViewportMode mode)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	std::string vertex_shader;
	std::string fragment_shader;

	// std::string vertex_shader = kernel_from_args(vertex, mode)
	// 	.compile(profiles::glsl_450);

	// std::string fragment_shader = kernel_from_args(fragment, mode)
	// 	.compile(profiles::glsl_450);

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	pipelines[mode] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void ViewportRenderGroup::configure_pipelines(DeviceResourceCollection &drc)
{
	for (int32_t i = 0; i < eCount; i++)
		configure_pipeline_mode(drc, (ViewportMode) i);
}

// Rendering
void ViewportRenderGroup::render(const RenderingInfo &info, Viewport &viewport)
{
	auto &cmd = info.cmd;
	auto &scene = info.device_scene;

	// Configure the rendering extent
	littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(viewport.extent));

	viewport.handle_input(info.window);

	auto rpbi = littlevk::default_rp_begin_info <2> (render_pass,
		viewport.framebuffers[info.operation.index],
		viewport.extent);

	cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

	auto &ppl = pipelines[viewport.mode];

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

	solid_t <MVP> mvp;

	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);
	auto m_id = uniform_field(MVP, id);
	auto m_selected = uniform_field(MVP, selected);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(viewport.aperature);
	mvp[m_view] = viewport.transform.to_view_matrix();
	mvp[m_selected] = viewport.selected;

	for (size_t i = 0; i < scene.uuids.size(); i++) {
		auto &mesh = scene.meshes[i];
		mvp[m_id] = scene.uuids[i];

		cmd.pushConstants <solid_t <MVP>> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);
		cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
		cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
	}

	cmd.endRenderPass();

	littlevk::transition(info.cmd, viewport.targets[info.operation.index],
		vk::ImageLayout::ePresentSrcKHR,
		vk::ImageLayout::eShaderReadOnlyOptimal);
}