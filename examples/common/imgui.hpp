#pragma once

#include "vulkan_resources.hpp"

// Rendering context using RAII
struct ImGuiRenderContext {
	const vk::CommandBuffer &cmd;

	ImGuiRenderContext(const vk::CommandBuffer &);
	~ImGuiRenderContext();
};

// Configure vulkan device resource collection with ImGui
void configure_imgui(VulkanResources &, const vk::RenderPass &);

// Generate a ImGui descriptor set for an image
vk::DescriptorSet imgui_texture_descriptor(const vk::Sampler &, const vk::ImageView &, const vk::ImageLayout &);

void shutdown_imgui();
