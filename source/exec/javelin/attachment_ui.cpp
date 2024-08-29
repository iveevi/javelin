#include "attachment_ui.hpp"
#include "source/exec/javelin/global_context.hpp"

// Construction
AttachmentUI::AttachmentUI(const std::unique_ptr <GlobalContext> &global_context)
		: gctx(*global_context)
{
	configure_render_pass();
	configure_framebuffers();
	imgui_initialize_vulkan(gctx.drc, render_pass);
}

void AttachmentUI::configure_render_pass()
{
	auto &drc = gctx.drc;
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();
}

void AttachmentUI::configure_framebuffers()
{
	auto &drc = gctx.drc;
	auto &swapchain = drc.swapchain;
	
	littlevk::FramebufferGenerator generator {
		drc.device, render_pass,
		drc.window.extent, drc.dal
	};

	for (size_t i = 0; i < swapchain.images.size(); i++)
		generator.add(swapchain.image_views[i]);

	framebuffers = generator.unpack();
}

// Rendering
void AttachmentUI::render(const RenderState &rs)
{
	const auto &rpbi = littlevk::default_rp_begin_info <1> (render_pass, framebuffers[rs.sop.index], gctx.drc.window);
	
	rs.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

	for (auto &ftn : callbacks)
		ftn();
	
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), rs.cmd);

	rs.cmd.endRenderPass();
}

Attachment AttachmentUI::attachment()
{
	return Attachment {
		.renderer = std::bind(&AttachmentUI::render, this, std::placeholders::_1),
		.resizer = std::bind(&AttachmentUI::configure_framebuffers, this),
		.user = this
	};
}