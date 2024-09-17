#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <littlevk/littlevk.hpp>

struct InteractiveWindow : littlevk::Window {
	InteractiveWindow() = default;
	InteractiveWindow(const littlevk::Window &);

	bool key_pressed(int) const;
};

struct DeviceResourceCollectionInfo {
	vk::PhysicalDevice phdev;
	std::string title;
	vk::Extent2D extent;
	std::vector <const char *> extensions;
};

class DeviceResourceCollection {
	void configure_device(const std::vector <const char *> &);
public:
	vk::PhysicalDevice phdev;
	vk::SurfaceKHR surface;
	vk::Device device;
	vk::Queue graphics_queue;
	vk::Queue present_queue;
	vk::CommandPool command_pool;
	vk::DescriptorPool descriptor_pool;
	vk::PhysicalDeviceMemoryProperties memory_properties;

	littlevk::Swapchain swapchain;
	littlevk::Deallocator dal;

	InteractiveWindow window;

	littlevk::LinkedDeviceAllocator <> allocator();
	littlevk::LinkedDevices combined();
	littlevk::LinkedCommandQueue commander();

	static DeviceResourceCollection from(const DeviceResourceCollectionInfo &);
};

namespace imgui {

// Configure vulkan device resource collection with ImGui
void configure_vulkan(DeviceResourceCollection &, const vk::RenderPass &);

// Generate a ImGui descriptor set for an image
vk::DescriptorSet add_vk_texture(const vk::Sampler &, const vk::ImageView &, const vk::ImageLayout &);

} // namespace imgui