#pragma once

#include <cstdint>
#include <cstdlib>
#include <filesystem>

#include <vulkan/vulkan.hpp>

#include <wrapped_types.hpp>

namespace jvl::core {

// Forward declarartions
class TextureBank;

struct Texture {
	uint8_t *data = nullptr;
	size_t width = 0;
	size_t height = 0;
	vk::Format format;

	static Texture from(const std::filesystem::path &);
	static Texture &from(TextureBank &, const std::filesystem::path &);
};

// Optional texture caches
class TextureBank : public wrapped::thread_safe_tree <std::string, Texture> {
	using wrapped::thread_safe_tree <std::string, Texture> ::thread_safe_tree;
};

} // namespace jvl::core