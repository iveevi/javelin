#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "rendering_info.hpp"

struct imgui_callback {
	int64_t global;
	std::function <void (const RenderingInfo &)> callback;
};

using imgui_callback_list = std::list <imgui_callback>;

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