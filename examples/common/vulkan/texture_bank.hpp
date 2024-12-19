#pragma once

#include <string>

#include <littlevk/littlevk.hpp>

#include <core/wrapped.hpp>

#include "texture.hpp"

namespace jvl::gfx::vulkan {

class TextureBank : public wrapped::thread_safe_tree <std::string, littlevk::Image> {
	// Textures which are yet to finish transitions
	std::set <std::string> processing;
public:
	using wrapped::thread_safe_tree <std::string, littlevk::Image> ::thread_safe_tree;

	bool ready(const std::string &) const;

	void mark_ready(const std::string &);

	bool upload(littlevk::LinkedDeviceAllocator <>,
		littlevk::LinkedCommandQueue,
		const std::string &,
		const core::Texture &);
	
	bool upload(littlevk::LinkedDeviceAllocator <>,
		const vk::CommandBuffer &,
		const std::string &,
		const core::Texture &);
};

} // namespace jvl::gfx::vulkan