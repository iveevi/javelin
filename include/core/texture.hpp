#pragma once

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <map>

namespace jvl::core {

// Forward declarartions
struct TextureBank;

struct Texture {
	uint8_t *data = nullptr;
	size_t width = 0;
	size_t height = 0;
	vk::Format format;

	static Texture from(const std::filesystem::path &);
	static Texture &from(TextureBank &, const std::filesystem::path &);
};

// Optional texture caches
struct TextureBank : public std::map <std::string, Texture> {
	using std::map <std::string, Texture> ::map;
};

} // namespace jvl::core