#pragma once

#include <vector>

#include <bestd/optional.hpp>

#include "interactive_window.hpp"

class VulkanResources {
	void configure_device(const std::vector <const char *> &,
			      const bestd::optional <vk::PhysicalDeviceFeatures2KHR> &);
public:
	vk::PhysicalDevice phdev;
	vk::SurfaceKHR surface;
	vk::Device device;
	vk::Queue graphics_queue;
	vk::Queue present_queue;
	vk::CommandPool command_pool;
	vk::DescriptorPool descriptor_pool;
	vk::PhysicalDeviceMemoryProperties memory_properties;

	// TODO: collect queue family indices...
	uint32_t graphics_index;

	littlevk::Swapchain swapchain;
	littlevk::Deallocator dal;

	InteractiveWindow window;

	littlevk::LinkedDeviceAllocator <> allocator();
	littlevk::LinkedDevices combined();
	littlevk::LinkedCommandQueue commander();

	vk::CommandBuffer new_command_buffer();

	// TODO: duplication method for another thread
	template <typename F, typename G>
	void swapchain_render_loop(const F &render, const G &resize) {
		littlevk::swapchain_render_loop(device,
			graphics_queue,
			present_queue,
			command_pool,
			window,
			swapchain,
			dal,
			render,
			resize);
	}

	static VulkanResources from(const vk::PhysicalDevice &,
		const std::string &,
		const vk::Extent2D &,
		const std::vector <const char *> &,
		const bestd::optional <vk::PhysicalDeviceFeatures2KHR> & = std::nullopt);
};