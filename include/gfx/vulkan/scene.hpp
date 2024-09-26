#pragma once

#include <littlevk/littlevk.hpp>

#include "triangle_mesh.hpp"
#include "../cpu/scene.hpp"

namespace jvl::gfx::vulkan {

// TODO: separate header
// Uber material structure
struct Material {
	// TODO: variant of different modes of materials... with flags
	struct uber_x {
		float3 kd;
		float3 ks;
		float roughness;
	};

	littlevk::Buffer uber;

	static std::optional <Material> from(littlevk::LinkedDeviceAllocator <> allocator, const core::Material &material) {
		uber_x info;

		if (auto opt_kd = material.values.get(core::Material::diffuse_key).as <float3> ())
			info.kd = opt_kd.value();

		if (auto opt_ks = material.values.get(core::Material::specular_key).as <float3> ())
			info.ks = opt_ks.value();

		if (auto opt_roughness = material.values.get(core::Material::roughness_key).as <float> ())
			info.roughness = opt_roughness.value();

		return Material {
			.uber = allocator.buffer(&info, sizeof(info),
				vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eUniformBuffer)
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