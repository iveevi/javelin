#include "core/device_resource_collection.hpp"

namespace jvl::core {

// DeviceResourceCollection implementations
littlevk::LinkedDeviceAllocator <> DeviceResourceCollection::allocator()
{
	return littlevk::bind(device, memory_properties, dal);
}

littlevk::LinkedDevices DeviceResourceCollection::combined()
{
	return littlevk::bind(phdev, device);
}

littlevk::LinkedCommandQueue DeviceResourceCollection::commander()
{
	return littlevk::bind(device, command_pool, graphics_queue);
}

vk::CommandBuffer DeviceResourceCollection::new_command_buffer()
{
	return device.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo {
			command_pool,
			vk::CommandBufferLevel::ePrimary, 1
		}).front();
}

void DeviceResourceCollection::configure_device(const std::vector <const char *> &EXTENSIONS)
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

DeviceResourceCollection DeviceResourceCollection::from(const DeviceResourceCollectionInfo &info)
{
	DeviceResourceCollection drc;

	drc.phdev = info.phdev;
	std::tie(drc.surface, drc.window) = littlevk::surface_handles(info.extent, info.title.c_str());
	drc.configure_device(info.extensions);

	return drc;
}


} // namespace jvl::engine