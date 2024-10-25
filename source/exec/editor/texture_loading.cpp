#include "texture_loading.hpp"
	
TextureLoadingWorker::TextureLoadingWorker(DeviceResourceCollection &drc,
					   TextureBank &host_bank_,
					   gfx::vulkan::TextureBank &device_bank_,
					   wrapped::thread_safe_queue <TextureTransitionUnit> &transition_queue_)
		: allocator(drc.allocator()),
		host_bank(host_bank_),
		device_bank(device_bank_),
		active(true),
		transition_queue(transition_queue_)
{
	auto ftn = std::bind(&TextureLoadingWorker::loop, this);
	worker = std::thread(ftn);

	auto command_pool_info = vk::CommandPoolCreateInfo()
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(drc.graphics_index);

	// Thread local command pool	
	command_pool = allocator.device.createCommandPool(command_pool_info);
}

TextureLoadingWorker::~TextureLoadingWorker()
{
	active = false;
	worker.join();
}

void TextureLoadingWorker::loop()
{
	while (active) {
		if (!items.size())
			continue;
		
		auto unit = items.pop();

		// Rest of the stuff...
		auto texture = core::Texture::from(host_bank, unit.source);

		auto allocate_info = vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(command_pool);

		auto cmd = allocator.device.allocateCommandBuffers(allocate_info).front();

		if (device_bank.upload(allocator, cmd, unit.source, texture)) {
			TextureTransitionUnit transition { cmd, unit.source };
			transition_queue.push(transition);
		}

		processing.erase(unit.source);
	}
}

void TextureLoadingWorker::push(const TextureLoadingUnit &unit)
{
	items.push(unit);
	processing.add(unit.source);
}

bool TextureLoadingWorker::pending(const std::string &source)
{
	return processing.contains(source);
}