#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "rendering_info.hpp"

using imgui_callback = std::function <void (const RenderingInfo &)>;
using imgui_callback_list = std::vector <imgui_callback>;

// Renders to the swapchain images
struct ImGuiRenderGroup {
	vk::RenderPass render_pass;
	std::vector <vk::Framebuffer> framebuffers;

	// Constructors
	ImGuiRenderGroup() = default;
	ImGuiRenderGroup(DeviceResourceCollection &);

	void resize(DeviceResourceCollection &);
	void render(const RenderingInfo &, const imgui_callback_list &);
};