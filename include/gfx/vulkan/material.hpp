#pragma once

#include "../../core/adaptive_descriptor.hpp"
#include "../../core/material.hpp"
#include "material_flags.hpp"

namespace jvl::gfx::vulkan {

struct Material : core::Material {
	MaterialFlags flags;

	core::DescriptorTable descriptors;

	Material(const core::Material &);
};

} // namespace jvl::gfx::vulkan