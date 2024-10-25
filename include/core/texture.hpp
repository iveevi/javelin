#pragma once

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <mutex>

#include <vulkan/vulkan.hpp>

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
// TODO: thread-safe tree
class TextureBank : std::map <std::string, Texture> {
	std::mutex lock;
	
	using super = std::map <std::string, Texture>;
public:
	bool contains(const std::string &key) {
		std::lock_guard guard(lock);
		return super::contains(key);
	}

	Texture &operator[](const std::string &key) {
		std::lock_guard guard(lock);
		return super::operator[](key);
	}
};

} // namespace jvl::core