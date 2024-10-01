#pragma once

#include "device_resource_collection.hpp"

namespace jvl::engine {

// Configure vulkan device resource collection with ImGui
void configure_imgui(DeviceResourceCollection &, const vk::RenderPass &);

// Generate a ImGui descriptor set for an image
vk::DescriptorSet imgui_texture_descriptor(const vk::Sampler &, const vk::ImageView &, const vk::ImageLayout &);

} // namespace jvl::engine