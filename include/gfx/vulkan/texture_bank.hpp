#pragma once

#include <string>
#include <map>

#include <littlevk/littlevk.hpp>

#include "../../core/device_resource_collection.hpp"
#include "../../core/texture.hpp"
#include "../../wrapped_types.hpp"

namespace jvl::gfx::vulkan {

struct TextureBank : public std::map <std::string, littlevk::Image> {
	using std::map <std::string, littlevk::Image> ::map;

	void upload(littlevk::LinkedDeviceAllocator <>,
		littlevk::LinkedCommandQueue,
		const std::string &,
		const core::Texture &);
	
	void upload(littlevk::LinkedDeviceAllocator <>,
		littlevk::LinkedCommandQueue,
		const std::string &,
		const core::Texture &,
		wrapped::thread_safe_queue <vk::CommandBuffer> &);
};

} // namespace jvl::gfx::vulkan