#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "logging.hpp"
#include "core/texture.hpp"

namespace jvl::core {

MODULE(texture);

Texture Texture::from(const std::filesystem::path &path)
{
	Texture result;

	stbi_set_flip_vertically_on_load(true);

	int width;
	int height;
	int channels;

	JVL_ASSERT(std::filesystem::exists(path), "failed to load path {}", path.string());

	std::string file = path.string();

	result.data = stbi_load(file.c_str(), &width, &height, &channels, 4);
	JVL_ASSERT(result.data, "failed to load texture from {}", path.string());
	result.width = width;
	result.height = height;

	fmt::println("  loaded texture {} (@{}) with dimensions: ({}, {}, {})",
		path.string(), (void *) result.data, width, height, channels);

	if (channels == 4)
		result.format = eRGBA;
	else
		result.format = eRGB;

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