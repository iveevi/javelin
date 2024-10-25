#include <zlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYEXR_USE_ZLIB 1
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

#include "logging.hpp"
#include "core/texture.hpp"

namespace jvl::core {

MODULE(texture);

Texture Texture::from(const std::filesystem::path &path)
{
	Texture result;

	stbi_set_flip_vertically_on_load(true);

	JVL_ASSERT(std::filesystem::exists(path), "failed to load path {}", path.string());

	std::string file = path.string();
	std::string extension = path.extension();
	fmt::println("image ext: {}", extension);
	if (extension == ".exr") {
		// TODO: unquantized version

		float *ptr;
		int width;
		int height;
		const char *error = nullptr;

		int ret = LoadEXR(&ptr, &width, &height, file.c_str(), &error);
		if (ret != TINYEXR_SUCCESS) {
			if (error)
				JVL_ABORT("failed to load (EXR) image: {}", error);

			JVL_ABORT("failed to load (EXR) image");
		}

		result.width = width;
		result.height = height;
		result.data = new uint8_t[4 * width * height];
		result.format = vk::Format::eR8G8B8A8Unorm;

		// Copy and quantize the data
		for (size_t i = 0; i < 4 *  width * height; i++) {
			result.data[i] = 255.0f * ptr[i];
		}

		fmt::println("loaded exr image with {} x {}", width, height);
		free(ptr);
		
		return result;
	}

	// Default generic
	int width;
	int height;
	int channels;

	result.data = stbi_load(file.c_str(), &width, &height, &channels, 4);
	JVL_ASSERT(result.data, "failed to load texture from {}", path.string());
	result.width = width;
	result.height = height;

	fmt::println("  loaded texture {} (@{}) with dimensions: ({}, {}, {})",
		path.string(), (void *) result.data, width, height, channels);

	result.format = vk::Format::eR8G8B8A8Unorm;

	return result;
}

Texture &Texture::from(TextureBank &bank, const std::filesystem::path &path)
{
	if (bank.contains(path))
		return bank[path];

	bank[path] = Texture::from(path);
	return bank[path];
}

} // namespace jvl::core