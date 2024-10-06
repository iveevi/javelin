#pragma once

#include <littlevk/littlevk.hpp>

#include "../../core/texture.hpp"
#include "../../core/device_resource_collection.hpp"
#include "../../logging.hpp"
#include "../cpu/scene.hpp"
#include "triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum class MaterialFlags : uint64_t {
	eNone = 0x0,
	eAlbedoSampler = 0x1,
	eSpecularSampler = 0x10,
	eRoughnessSampler = 0x100,
};

[[gnu::always_inline]]
inline bool enabled(MaterialFlags one, MaterialFlags two)
{
	return (uint64_t(one) & uint64_t(two)) == uint64_t(two);
}

[[gnu::always_inline]]
inline MaterialFlags operator|(MaterialFlags one, MaterialFlags two)
{
	return MaterialFlags(uint64_t(one) | uint64_t(two));
}

// TODO: separate header
// Uber material structure
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
	littlevk::Image albedo;

	static std::optional <Material> from(core::DeviceResourceCollection &drc, const core::Material &material) {
		MODULE(vulkan-material-from);

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
};

struct Scene {
	std::vector <int64_t> uuids;
	std::vector <TriangleMesh> meshes;
	std::vector <Material> materials;

	static Scene from(core::DeviceResourceCollection &drc, const cpu::Scene &other) {
		Scene result;
		result.uuids = other.uuids;
		
		for (auto &m : other.meshes) {
			auto vkm = TriangleMesh::from(drc.allocator(), m).value();
			result.meshes.push_back(vkm);
		}

		for (auto &m : other.materials) {
			auto vkm = Material::from(drc, m).value();
			result.materials.push_back(vkm);
		}

		return result;
	}
};

} // namespace jvl::gfx::vulkan