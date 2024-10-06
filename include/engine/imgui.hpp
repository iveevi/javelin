#pragma once

#include "../core/device_resource_collection.hpp"

namespace jvl::engine {

// Rendering context using RAII
struct ImGuiRenderContext {
	const vk::CommandBuffer &cmd;

	ImGuiRenderContext(const vk::CommandBuffer &);
	~ImGuiRenderContext();
};

// Configure vulkan device resource collection with ImGui
void configure_imgui(core::DeviceResourceCollection &, const vk::RenderPass &);

// Generate a ImGui descriptor set for an image
vk::DescriptorSet imgui_texture_descriptor(const vk::Sampler &, const vk::ImageView &, const vk::ImageLayout &);

} // namespace jvl::engine