#include "vulkan_resources.hpp"

// DeviceResourceCollection implementations
littlevk::LinkedDeviceAllocator <> VulkanResources::allocator()
{
	return littlevk::bind(device, memory_properties, dal);
}

littlevk::LinkedDevices VulkanResources::combined()
{
	return littlevk::bind(phdev, device);
}

littlevk::LinkedCommandQueue VulkanResources::commander()
{
	return littlevk::bind(device, command_pool, graphics_queue);
}

vk::CommandBuffer VulkanResources::new_command_buffer()
{
	return device.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo {
			command_pool,
			vk::CommandBufferLevel::ePrimary, 1
		}).front();
}

void VulkanResources::configure_device(const std::vector <const char *> &EXTENSIONS)
{
	auto queue_family = littlevk::find_queue_families(phdev, surface);

	graphics_index = queue_family.graphics;

	memory_properties = phdev.getMemoryProperties();
	device = littlevk::device(phdev, queue_family, EXTENSIONS);
	dal = littlevk::Deallocator(device);

	swapchain = combined().swapchain(surface, queue_family);

	graphics_queue = device.getQueue(queue_family.graphics, 0);
	present_queue = device.getQueue(queue_family.present, 0);

	command_pool = littlevk::command_pool(device,
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family.graphics).unwrap(dal);

	// Configuring the descriptor pool
	std::vector <vk::DescriptorPoolSize> sizes {
		vk::DescriptorPoolSize {
			vk::DescriptorType::eUniformBuffer,
			2
		}
	};

	descriptor_pool = littlevk::descriptor_pool(device,
		vk::DescriptorPoolCreateInfo {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1 << 20,
			sizes
		}).unwrap(dal);
}

VulkanResources VulkanResources::from(const std::string &title,
							const vk::Extent2D &extent,
							const std::vector <const char *> &extensions)
{
	VulkanResources drc;
	
	auto predicate = [&](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, extensions);
	};

	drc.phdev = littlevk::pick_physical_device(predicate);
	std::tie(drc.surface, drc.window) = littlevk::surface_handles(extent, title.c_str());
	drc.configure_device(extensions);

	return drc;
}
