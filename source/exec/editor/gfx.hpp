#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <littlevk/littlevk.hpp>

#include <engine/device_resource_collection.hpp>

using namespace jvl;
using namespace jvl::engine;

namespace imgui {

// Configure vulkan device resource collection with ImGui
void configure_vulkan(DeviceResourceCollection &, const vk::RenderPass &);

// Generate a ImGui descriptor set for an image
vk::DescriptorSet add_vk_texture(const vk::Sampler &, const vk::ImageView &, const vk::ImageLayout &);

} // namespace imgui