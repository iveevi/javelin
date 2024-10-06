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

		std::string path = kd.as <std::string> ();
		fmt::println("uploading texture {} to device", path);

		auto texture = core::Texture::from(path);

		littlevk::ImageCreateInfo image_info {
			uint32_t(texture.width),
			uint32_t(texture.height),
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageUsageFlagBits::eSampled
			| vk::ImageUsageFlagBits::eTransferDst,
			vk::ImageAspectFlagBits::eColor,
			vk::ImageType::e2D,
			vk::ImageViewType::e2D
		};

		littlevk::Buffer staging;

		std::tie(staging, result.albedo) = drc.allocator()
			.buffer(texture.data,
				4 * texture.width * texture.height * sizeof(uint8_t),
				vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eTransferSrc)
			.image(image_info);

		drc.commander().submit_and_wait([&](const vk::CommandBuffer &cmd) {
			result.albedo.transition(cmd, vk::ImageLayout::eTransferDstOptimal);

			littlevk::copy_buffer_to_image(cmd,
				result.albedo,
				staging,
				vk::ImageLayout::eTransferDstOptimal);

			result.albedo.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
		});
	} else {
		JVL_ASSERT_PLAIN(kd.is <float3> ());
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