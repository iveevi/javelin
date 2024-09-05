#include "attachment_viewport.hpp"

using namespace jvl;
using namespace jvl::ire;
using namespace jvl::core;
using namespace jvl::gfx;

// Shader sources
struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"mvp",
			field <"model"> (model),
			field <"view"> (view),
			field <"proj"> (proj)
		);
	}
};

// TODO: with regular input/output semantics?
void vertex()
{
	layout_in <vec3> position(0);
	layout_out <vec3> color(0);

	push_constant <mvp> mvp;

	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;
	color = position;
}

void fragment()
{
	layout_in <vec3> position(0);
	layout_out <vec4> fragment(0);

	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
}

// Concrete push constants aggregate
struct MVP {
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};

// Construction
AttachmentViewport::AttachmentViewport(const std::unique_ptr <GlobalContext> &global_context)
		: gctx(*global_context)
{
	configure_render_pass();
	configure_framebuffers();
	configure_pipelines();
}

void AttachmentViewport::handle_input()
{
	static constexpr float speed = 50.0f;
	static float last_time = 0.0f;

	auto &window = gctx.drc.window;

	float delta = speed * float(glfwGetTime() - last_time);
	last_time = glfwGetTime();

	jvl::float3 velocity(0.0f);
	if (window.key_pressed(GLFW_KEY_S))
		velocity.z -= delta;
	else if (window.key_pressed(GLFW_KEY_W))
		velocity.z += delta;

	if (window.key_pressed(GLFW_KEY_D))
		velocity.x -= delta;
	else if (window.key_pressed(GLFW_KEY_A))
		velocity.x += delta;

	if (window.key_pressed(GLFW_KEY_E))
		velocity.y += delta;
	else if (window.key_pressed(GLFW_KEY_Q))
		velocity.y -= delta;

	transform.translate += transform.rotation.rotate(velocity);
}

void AttachmentViewport::configure_render_pass()
{
	auto &drc = gctx.drc;
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
	.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
	.add_attachment(littlevk::default_depth_attachment())
	.add_subpass(vk::PipelineBindPoint::eGraphics)
		.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
		.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.done();
}

void AttachmentViewport::configre_normal_pipeline()
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto &drc = gctx.drc;

	std::string vertex_shader = kernel_from_args(vertex).synthesize(jvl::profiles::opengl_450);
	std::string fragment_shader = kernel_from_args(fragment).synthesize(jvl::profiles::opengl_450);

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	pipelines[eNormal] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <MVP> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void AttachmentViewport::configure_pipelines()
{
	configre_normal_pipeline();
}

void AttachmentViewport::configure_framebuffers()
{
	auto &drc = gctx.drc;
	auto &swapchain = drc.swapchain;

	targets.clear();
	framebuffers.clear();
	imgui_descriptors.clear();

	littlevk::Image depth = drc.allocator()
		.image(drc.window.extent,
			vk::Format::eD32Sfloat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth);

	targets.clear();
	for (size_t i = 0; i < drc.swapchain.images.size(); i++) {
		targets.push_back(drc.allocator()
			.image(drc.window.extent,
				vk::Format::eB8G8R8A8Unorm,
				vk::ImageUsageFlagBits::eColorAttachment
					| vk::ImageUsageFlagBits::eSampled,
				vk::ImageAspectFlagBits::eColor)
		);
	}

	littlevk::FramebufferGenerator generator {
		drc.device, render_pass,
		drc.window.extent, drc.dal
	};

	for (size_t i = 0; i < drc.swapchain.images.size(); i++)
		generator.add(targets[i].view, depth.view);

	framebuffers = generator.unpack();

	vk::Sampler sampler = littlevk::SamplerAssembler(drc.device, drc.dal);
	for (const littlevk::Image &image : targets) {
		vk::DescriptorSet dset = imgui_add_vk_texture(sampler, image.view,
				vk::ImageLayout::eShaderReadOnlyOptimal);

		imgui_descriptors.push_back(dset);
	}

	auto transition = [&](const vk::CommandBuffer &cmd) {
		for (auto &image : targets)
			image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
	};

	drc.commander().submit_and_wait(transition);
}

// Rendering
void AttachmentViewport::render(const RenderState &rs)
{
	handle_input();

	const auto &rpbi = littlevk::default_rp_begin_info <2> (render_pass, framebuffers[rs.sop.index], gctx.drc.window);

	rs.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

	auto &ppl = pipelines[AttachmentViewport::eNormal];

	rs.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

	MVP mvp;
	mvp.model = jvl::float4x4::identity();
	mvp.proj = jvl::core::perspective(aperature);
	mvp.view = transform.to_view_matrix();

	rs.cmd.pushConstants <MVP> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);

	for (const auto &vmesh : meshes) {
		rs.cmd.bindVertexBuffers(0, vmesh.vertices.buffer, { 0 });
		rs.cmd.bindIndexBuffer(vmesh.triangles.buffer, 0, vk::IndexType::eUint32);
		rs.cmd.drawIndexed(vmesh.count, 1, 0, 0, 0);
	}

	rs.cmd.endRenderPass();

	littlevk::transition(rs.cmd, targets[rs.frame], vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eShaderReadOnlyOptimal);
}

// ImGui rendering
void AttachmentViewport::render_imgui()
{
	bool preview_open = true;
	ImGui::Begin("Preview", &preview_open, ImGuiWindowFlags_MenuBar);

	if (ImGui::BeginMenuBar()) {
		if (ImGui::Button("Render")) {
			auto rtx = gctx.attachment_user <AttachmentRaytracingCPU> ();
			rtx->trigger(transform);
		}

		ImGui::EndMenuBar();
	}

	viewport_region.active = ImGui::IsWindowFocused();

	ImVec2 size = ImGui::GetContentRegionAvail();
	ImGui::Image(imgui_descriptors[gctx.frame.index], size);

	ImVec2 viewport = ImGui::GetItemRectSize();
	aperature.aspect = viewport.x/viewport.y;

	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();

	// TODO: should be local
	viewport_region.min = { min.x, min.y };
	viewport_region.max = { max.x, max.y };

	ImGui::End();
}

Attachment AttachmentViewport::attachment()
{
	return Attachment {
		.renderer = std::bind(&AttachmentViewport::render, this, std::placeholders::_1),
		.resizer = std::bind(&AttachmentViewport::configure_framebuffers, this),
		.user = this
	};
}
