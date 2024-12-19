#include "material.hpp"

namespace jvl::gfx::vulkan {
	
Material::Material(const core::Material &other)
		: core::Material(other)
{
	// Collect the flags
	flags = MaterialFlags::eNone;

	// Diffuse (albedo)
	auto kd = values.get(core::Material::diffuse_key);
	if (kd && kd->is <core::texture> ())
		flags = flags | MaterialFlags::eAlbedoSampler;

	// Specular
	auto ks = values.get(core::Material::specular_key);
	if (ks && ks->is <core::texture> ())
		flags = flags | MaterialFlags::eSpecularSampler;

	// Roughness
	auto roughness = values.get(core::Material::roughness_key);
	if (roughness && roughness->is <core::texture> ())
		flags = flags | MaterialFlags::eRoughnessSampler;
}

} // namespace jvl::gfx::vulkan