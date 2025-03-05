#include <zlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYEXR_USE_ZLIB 1
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

#include <common/logging.hpp>

#include "texture.hpp"

MODULE(texture);

Texture Texture::from(const std::filesystem::path &path)
{
	Texture result;

	stbi_set_flip_vertically_on_load(true);

	JVL_ASSERT(std::filesystem::exists(path), "failed to load path {}", path.string());

	std::string file = path.string();
	std::string extension = path.extension();
	if (extension == ".exr") {
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
		result.format = vk::Format::eR32G32B32A32Sfloat;
		result.data = (uint8_t *) ptr;
		
		JVL_INFO("loaded texture (EXR) {} (@{}) with dimensions: ({}, {})",
			path.string(), (void *) result.data, width, height);
		
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

	JVL_INFO("loaded texture {} (@{}) with dimensions: ({}, {}, {})",
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