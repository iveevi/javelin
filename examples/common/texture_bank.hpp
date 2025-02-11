#pragma once

#include <string>

#include <littlevk/littlevk.hpp>

#include <bestd/tree.hpp>

#include "texture.hpp"

class VulkanTextureBank : public bestd::tree <std::string, littlevk::Image> {
	// Textures which are yet to finish transitions
	std::set <std::string> processing;
public:
	using bestd::tree <std::string, littlevk::Image> ::tree;

	bool ready(const std::string &) const;

	void mark_ready(const std::string &);

	bool upload(littlevk::LinkedDeviceAllocator <>,
		littlevk::LinkedCommandQueue,
		const std::string &,
		const Texture &);
	
	bool upload(littlevk::LinkedDeviceAllocator <>,
		const vk::CommandBuffer &,
		const std::string &,
		const Texture &);
};