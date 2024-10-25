#pragma once

#include <string>
#include <map>

#include <littlevk/littlevk.hpp>

#include "../../core/device_resource_collection.hpp"
#include "../../core/texture.hpp"
#include "../../wrapped_types.hpp"

namespace jvl::gfx::vulkan {

class TextureBank : public std::map <std::string, littlevk::Image> {
	// Textures which are yet to finish transitions
	std::set <std::string> processing;
public:
	using std::map <std::string, littlevk::Image> ::map;

	bool ready(const std::string &) const;

	void mark_ready(const std::string &);

	void upload(littlevk::LinkedDeviceAllocator <>,
		littlevk::LinkedCommandQueue,
		const std::string &,
		const core::Texture &);
	
	void upload(littlevk::LinkedDeviceAllocator <>,
		const vk::CommandBuffer &,
		const std::string &,
		const core::Texture &);
};

} // namespace jvl::gfx::vulkan