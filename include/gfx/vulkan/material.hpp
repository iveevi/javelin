#pragma once

#include "../../core/messaging.hpp"
#include "../../core/device_resource_collection.hpp"
#include "../../core/material.hpp"
#include "../../core/texture.hpp"
#include "material_flags.hpp"
#include "texture_bank.hpp"

namespace jvl::gfx::vulkan {

// Potential texture states
struct UnloadedTexture {
	std::string path;
};

struct ReadyTexture {
	std::string path;
};

struct LoadedTexture : littlevk::Image {
	std::string path;

	LoadedTexture(const littlevk::Image &image, const std::string &path_)
		: littlevk::Image(image), path(path_) {}
};

template <typename T>
using transitional_material_property = wrapped::variant <T,
	UnloadedTexture,	// Texture has not been loaded
	ReadyTexture,		// Texture is loaded but not bound
	LoadedTexture		// Texture is loaded and bound to the material
>;

// Device-side material structure
struct Material {
	core::UUID uuid;

	// TODO: variant of different modes of materials... with flags
	struct uber_x {
		float3 kd;
		float3 ks;
		float roughness;
	};

	MaterialFlags flags;

	littlevk::Buffer uber;
	littlevk::Buffer specialized;

	transitional_material_property <float3> kd;

	static std::optional <Material> from(core::DeviceResourceCollection &,
		core::TextureBank &,
		TextureBank &,
		const core::Material &);
};

} // namespace jvl::gfx::vulkan