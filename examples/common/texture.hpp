#pragma once

#include <cstdint>
#include <cstdlib>
#include <filesystem>

#include <bestd/tree.hpp>

#include <vulkan/vulkan.hpp>

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
class TextureBank : public bestd::tree <std::string, Texture> {
	using bestd::tree <std::string, Texture> ::tree;
};