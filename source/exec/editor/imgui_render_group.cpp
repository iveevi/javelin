#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <engine/imgui.hpp>

#include "imgui_render_group.hpp"

ImGuiRenderGroup::ImGuiRenderGroup(DeviceResourceCollection &drc)
{
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	resize(drc);	
}

void ImGuiRenderGroup::resize(DeviceResourceCollection &drc)
{
	auto &swapchain = drc.swapchain;
	
	littlevk::FramebufferGenerator generator {
		drc.device, render_pass,
		drc.window.extent, drc.dal
	};

	for (size_t i = 0; i < swapchain.images.size(); i++)
		generator.add(swapchain.image_views[i]);

	framebuffers = generator.unpack();
}

void ImGuiRenderGroup::render(const RenderingInfo &info, const imgui_callback_list &callbacks)
{
	// Configure the rendering extent
	littlevk::viewport_and_scissor(info.cmd, littlevk::RenderArea(info.window));

	// Start the rendering pass
	littlevk::RenderPassBeginInfo(1)
		.with_render_pass(render_pass)
		.with_framebuffer(framebuffers[info.operation.index])
		.with_extent(info.window.extent)
		.clear_color(0, std::array <float, 4> { 0, 0, 0, 0 })
		.begin(info.cmd);

	{
		ImGuiRenderContext context(info.cmd);

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
		for (auto &ic : callbacks)
			ic.callback(info);
	}
	
	// Finish the rendering pass
	info.cmd.endRenderPass();
}