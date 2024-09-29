#pragma once

#include <cstdint>
#include <cstdlib>
#include <filesystem>

namespace jvl::core {

struct Texture {
	uint8_t *data = nullptr;
	size_t width = 0;
	size_t height = 0;
	
	enum Format {
		eRGB,
		eRGBA
	} format;

	static Texture from(const std::filesystem::path &);
};

} // namespace jvl::core