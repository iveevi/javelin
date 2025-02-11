#pragma once

#include "adaptive_descriptor.hpp"
#include "imported_material.hpp"

#include "material_flags.hpp"

struct Material : ImportedMaterial {
	MaterialFlags flags;
	DescriptorTable descriptors;
	
	Material(const ImportedMaterial &);
};