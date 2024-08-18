#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <littlevk/littlevk.hpp>

struct InteractiveWindow : littlevk::Window {
	InteractiveWindow() = default;

	InteractiveWindow(const littlevk::Window &win) : littlevk::Window(win) {}

	bool key_pressed(int key) const {
		return glfwGetKey(handle, key) == GLFW_PRESS;
	}
};

struct DeviceResourceCollection {
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

	auto allocator() {
		return littlevk::bind(device, memory_properties, dal);
	}

	auto combined() {
		return littlevk::bind(phdev, device);
	}

	auto commander() {
		return littlevk::bind(device, command_pool, graphics_queue);
	}

	template <typename ... Args>
	void configure_display(const Args &... args) {
		littlevk::Window win;
		std::tie(surface, win) = littlevk::surface_handles(args...);
		window = win;
	}

	void configure_device(const std::vector <const char *> &EXTENSIONS) {
		littlevk::QueueFamilyIndices queue_family = littlevk::find_queue_families(phdev, surface);

		memory_properties = phdev.getMemoryProperties();
		device = littlevk::device(phdev, queue_family, EXTENSIONS);
		dal = littlevk::Deallocator(device);

		graphics_queue = device.getQueue(queue_family.graphics, 0);
		present_queue = device.getQueue(queue_family.present, 0);

		command_pool = littlevk::command_pool(device,
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queue_family.graphics).unwrap(dal);

		swapchain = combined().swapchain(surface, queue_family);
	}
};

vk::DescriptorSet imgui_add_vk_texture(const vk::Sampler &, const vk::ImageView &, const vk::ImageLayout &);
void imgui_initialize_vulkan(DeviceResourceCollection &, const vk::RenderPass &);
