#pragma once

#include "interactive_window.hpp"

namespace jvl::engine {

// TODO: move to core?
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

} // namespace jvl::engine