#pragma once

#include <thread>
#include <atomic>
#include <functional>

#include <fmt/printf.h>

#include <core/texture.hpp>
#include <core/device_resource_collection.hpp>
#include <gfx/vulkan/texture_bank.hpp>
#include <wrapped_types.hpp>

#include "texture_transition.hpp"

using namespace jvl;
using namespace jvl::core;

struct TextureLoadingUnit {
	std::string source;
	bool to_device;
};

class TextureLoadingWorker {
	littlevk::LinkedDeviceAllocator <> allocator;

	core::TextureBank &host_bank;
	gfx::vulkan::TextureBank &device_bank;

	std::atomic_bool active;
	std::vector <std::thread> workers;
	wrapped::thread_safe_set <std::string> processing;
	wrapped::thread_safe_queue <TextureLoadingUnit> items;
	
	wrapped::thread_safe_queue <TextureTransitionUnit> &transition_queue;

	void loop(const vk::CommandPool &);
public:
	TextureLoadingWorker(DeviceResourceCollection &,
			     TextureBank &,
			     gfx::vulkan::TextureBank &,
			     wrapped::thread_safe_queue <TextureTransitionUnit> &);

	~TextureLoadingWorker();

	void push(const TextureLoadingUnit &);
	bool pending(const std::string &);
};