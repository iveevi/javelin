#pragma once

#include "../adaptive_descriptor.hpp"
#include "../material.hpp"

#include "material_flags.hpp"

namespace jvl::gfx::vulkan {

struct Material : core::Material {
	MaterialFlags flags;

	core::DescriptorTable descriptors;

	Material(const core::Material &);
};

} // namespace jvl::gfx::vulkan