#include "material.hpp"

Material::Material(const ImportedMaterial &other) : ImportedMaterial(other)
{
	// Collect the flags
	flags = MaterialFlags::eNone;

	// Diffuse (albedo)
	auto kd = values.get(ImportedMaterial::diffuse_key);
	if (kd && kd->is <texture> ())
		flags = flags | MaterialFlags::eAlbedoSampler;

	// Specular
	auto ks = values.get(ImportedMaterial::specular_key);
	if (ks && ks->is <texture> ())
		flags = flags | MaterialFlags::eSpecularSampler;

	// Roughness
	auto roughness = values.get(ImportedMaterial::roughness_key);
	if (roughness && roughness->is <texture> ())
		flags = flags | MaterialFlags::eRoughnessSampler;
}