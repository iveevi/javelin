#pragma once

#include <thread>
#include <atomic>
#include <functional>

#include <fmt/printf.h>

#include <core/texture.hpp>
#include <gfx/vulkan/texture_bank.hpp>
#include <wrapped_types.hpp>

#include "texture_transition.hpp"

using namespace jvl;
using namespace jvl::core;

struct TextureLoadingUnit {
	std::string source;
	bool to_device;
};

template <typename T>
class thread_safe_set : std::set <T> {
	std::mutex lock;

	using super = std::set <T>;
public:
	void add(const T &value) {
		std::lock_guard guard(lock);
		super::insert(value);
	}

	void erase(const T &value) {
		std::lock_guard guard(lock);
		super::erase(value);
	}

	bool contains(const T &value) {
		std::lock_guard guard(lock);
		return super::contains(value);
	}
};

class TextureLoadingWorker {
	littlevk::LinkedDeviceAllocator <> allocator;

	vk::CommandPool command_pool;

	core::TextureBank &host_bank;
	gfx::vulkan::TextureBank &device_bank;

	// TODO: multiple workers...
	std::thread worker;
	std::atomic_bool active;
	thread_safe_set <std::string> processing;
	wrapped::thread_safe_queue <TextureLoadingUnit> items;
	
	wrapped::thread_safe_queue <TextureTransitionUnit> &transition_queue;

	void loop() {
		while (active) {
			if (!items.size())
				continue;
			
			auto unit = items.pop();

			fmt::println("loading texture: {}", unit.source);

			// Rest of the stuff...
			auto texture = core::Texture::from(host_bank, unit.source);
			(void) texture;
			(void) device_bank;

			auto allocate_info = vk::CommandBufferAllocateInfo()
				.setCommandBufferCount(1)
				.setCommandPool(command_pool);

			auto cmd = allocator.device.allocateCommandBuffers(allocate_info).front();

			// TODO: make this thread safe as well...	
			device_bank.upload(allocator, cmd, unit.source, texture);

			TextureTransitionUnit transition { cmd, unit.source };
			transition_queue.push(transition);
		}
	}
public:
	TextureLoadingWorker(DeviceResourceCollection &drc,
			     TextureBank &host_bank_,
			     gfx::vulkan::TextureBank &device_bank_,
			     wrapped::thread_safe_queue <TextureTransitionUnit> &transition_queue_)
			: allocator(drc.allocator()),
			host_bank(host_bank_),
			device_bank(device_bank_),
			active(true),
			transition_queue(transition_queue_) {
		auto ftn = std::bind(&TextureLoadingWorker::loop, this);
		worker = std::thread(ftn);

		auto command_pool_info = vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(drc.graphics_index);

		// Thread local command pool	
		command_pool = allocator.device.createCommandPool(command_pool_info);
	}

	~TextureLoadingWorker() {
		active = false;
		worker.join();
	}

	void push(const TextureLoadingUnit &unit) {
		items.push(unit);
		processing.add(unit.source);
	}

	bool pending(const std::string &source) {
		return processing.contains(source);
	}
};