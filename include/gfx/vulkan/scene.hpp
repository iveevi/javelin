#pragma once

#include <littlevk/littlevk.hpp>

#include "../../core/device_resource_collection.hpp"
#include "../cpu/scene.hpp"
#include "triangle_mesh.hpp"
#include "material.hpp"

namespace jvl::gfx::vulkan {

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