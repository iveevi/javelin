#pragma once

#include "../../core/messaging.hpp"
#include "../../core/device_resource_collection.hpp"
#include "../../core/material.hpp"
#include "../../core/texture.hpp"
#include "material_flags.hpp"
#include "texture_bank.hpp"

namespace jvl::gfx::vulkan {

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

	// TODO: some deferred strategy
	wrapped::variant <float3, littlevk::Image> kd;

	static std::optional <Material> from(core::DeviceResourceCollection &,
		core::TextureBank &,
		TextureBank &,
		const core::Material &);
};

} // namespace jvl::gfx::vulkan