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
	if (kd.is <core::texture> ()) {
		result.flags = result.flags | MaterialFlags::eAlbedoSampler;
		result.kd = UnloadedTexture(kd.as <core::texture> ());
	} else {
		result.kd = kd.as <core::color3> ();
	}

	// Specular
	JVL_ASSERT_PLAIN(material.values.contains(core::Material::specular_key));
	auto ks = material.values.get(core::Material::specular_key).value();
	if (ks.is <core::texture> ())
		result.flags = result.flags | MaterialFlags::eSpecularSampler;
	else
		JVL_ASSERT_PLAIN(ks.is <core::color3> ());

	// Roughness
	JVL_ASSERT_PLAIN(material.values.contains(core::Material::roughness_key));
	auto roughness = material.values.get(core::Material::roughness_key).value();
	if (roughness.is <core::texture> ())
		result.flags = result.flags | MaterialFlags::eRoughnessSampler;
	else
		JVL_ASSERT_PLAIN(roughness.is <float> ());

	// Construct the Uber material form
	uber_x data_uber;

	if (enabled(result.flags, MaterialFlags::eAlbedoSampler))
		data_uber.kd = float3(0.5, 0.5, 0.5);
	else
		data_uber.kd = kd.as <core::color3> ();
	
	if (enabled(result.flags, MaterialFlags::eSpecularSampler))
		data_uber.ks = float3(0.5, 0.5, 0.5);
	else
		data_uber.ks = ks.as <core::color3> ();
	
	if (enabled(result.flags, MaterialFlags::eRoughnessSampler))
		data_uber.roughness = 0.1;
	else
		data_uber.roughness = roughness.as <float> ();

	result.uber = drc.allocator().buffer(&data_uber, sizeof(data_uber),
			vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eUniformBuffer);

	return result;
}

} // namespace jvl::gfx::vulkan