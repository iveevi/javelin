#pragma once

#include <littlevk/littlevk.hpp>

#include "../../logging.hpp"
#include "../cpu/scene.hpp"
#include "triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum MaterialFlags : uint64_t {
	eNone = 0x0,
	eAlbedoSampler = 0x1,
	eSpecularSampler = 0x10,
	eRoughnessSampler = 0x100,
};

[[gnu::always_inline]]
inline bool enabled(MaterialFlags one, MaterialFlags two)
{
	return (one & two) == two;
}

[[gnu::always_inline]]
inline MaterialFlags operator|(MaterialFlags one, MaterialFlags two)
{
	return MaterialFlags(uint64_t(one) | uint64_t(two));
}

// TODO: separate header
// Uber material structure
struct Material {
	// TODO: variant of different modes of materials... with flags
	struct uber_x {
		float3 kd;
		float3 ks;
		float roughness;
	};

	MaterialFlags flags;

	littlevk::Buffer uber;
	littlevk::Buffer specialized;

	static std::optional <Material> from(littlevk::LinkedDeviceAllocator <> allocator, const core::Material &material) {
		MODULE(vulkan-material-from);

		// Albedo / Diffuse
		MaterialFlags flags = eNone;

		JVL_ASSERT_PLAIN(material.values.contains(core::Material::diffuse_key));
		auto kd = material.values.get(core::Material::diffuse_key).value();
		if (kd.is <std::string> ())
			flags = flags | eAlbedoSampler;
		else
			JVL_ASSERT_PLAIN(kd.is <float3> ());

		// Specular
		JVL_ASSERT_PLAIN(material.values.contains(core::Material::specular_key));
		auto ks = material.values.get(core::Material::specular_key).value();
		if (ks.is <std::string> ())
			flags = flags | eSpecularSampler;
		else
			JVL_ASSERT_PLAIN(ks.is <float3> ());

		// Roughness
		JVL_ASSERT_PLAIN(material.values.contains(core::Material::roughness_key));
		auto roughness = material.values.get(core::Material::roughness_key).value();
		if (roughness.is <std::string> ())
			flags = flags | eRoughnessSampler;
		else
			JVL_ASSERT_PLAIN(roughness.is <float> ());

		// Construct the Uber material form
		uber_x info;

		if (enabled(flags, eAlbedoSampler))
			info.kd = float3(0.5, 0.5, 0.5);
		else
			info.kd = kd.as <float3> ();
		
		if (enabled(flags, eSpecularSampler))
			info.ks = float3(0.5, 0.5, 0.5);
		else
			info.ks = ks.as <float3> ();
		
		if (enabled(flags, eRoughnessSampler))
			info.roughness = 0.1;
		else
			info.roughness = roughness.as <float> ();

		return Material {
			.flags = flags,
			.uber = allocator.buffer(&info, sizeof(info),
				vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eUniformBuffer),
		};
	}
};

struct Scene {
	std::vector <int64_t> uuids;
	std::vector <TriangleMesh> meshes;
	std::vector <Material> materials;

	static Scene from(const littlevk::LinkedDeviceAllocator <> &allocator, const cpu::Scene &other) {
		Scene result;
		result.uuids = other.uuids;
		
		for (auto &m : other.meshes) {
			auto vkm = TriangleMesh::from(allocator, m).value();
			result.meshes.push_back(vkm);
		}

		for (auto &m : other.materials) {
			auto vkm = Material::from(allocator, m).value();
			result.materials.push_back(vkm);
		}

		return result;
	}
};

} // namespace jvl::gfx::vulkan