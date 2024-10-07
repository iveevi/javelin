#include "core/texture.hpp"
#include "gfx/vulkan/material.hpp"
#include "logging.hpp"

namespace jvl::gfx::vulkan {

MODULE(vulkan-material);

std::optional <Material> Material::from(core::DeviceResourceCollection &drc, const core::Material &material)
{
	Material result;

	result.uuid = core::new_uuid <Material> ();

	// Albedo / Diffuse
	result.flags = MaterialFlags::eNone;

	JVL_ASSERT_PLAIN(material.values.contains(core::Material::diffuse_key));
	auto kd = material.values.get(core::Material::diffuse_key).value();
	if (kd.is <std::string> ()) {
		result.flags = result.flags | MaterialFlags::eAlbedoSampler;
		result.kd = UnloadedTexture(kd.as <std::string> ());
	} else {
		result.kd = kd.as <float3> ();
	}

	// Specular
	JVL_ASSERT_PLAIN(material.values.contains(core::Material::specular_key));
	auto ks = material.values.get(core::Material::specular_key).value();
	if (ks.is <std::string> ())
		result.flags = result.flags | MaterialFlags::eSpecularSampler;
	else
		JVL_ASSERT_PLAIN(ks.is <float3> ());

	// Roughness
	JVL_ASSERT_PLAIN(material.values.contains(core::Material::roughness_key));
	auto roughness = material.values.get(core::Material::roughness_key).value();
	if (roughness.is <std::string> ())
		result.flags = result.flags | MaterialFlags::eRoughnessSampler;
	else
		JVL_ASSERT_PLAIN(roughness.is <float> ());

	// Construct the Uber material form
	uber_x info;

	if (enabled(result.flags, MaterialFlags::eAlbedoSampler))
		info.kd = float3(0.5, 0.5, 0.5);
	else
		info.kd = kd.as <float3> ();
	
	if (enabled(result.flags, MaterialFlags::eSpecularSampler))
		info.ks = float3(0.5, 0.5, 0.5);
	else
		info.ks = ks.as <float3> ();
	
	if (enabled(result.flags, MaterialFlags::eRoughnessSampler))
		info.roughness = 0.1;
	else
		info.roughness = roughness.as <float> ();

	result.uber = drc.allocator().buffer(&info, sizeof(info),
			vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eUniformBuffer);

	return result;
}

} // namespace jvl::gfx::vulkan