#pragma once

#include <littlevk/littlevk.hpp>

#include "../../core/device_resource_collection.hpp"
#include "../../core/texture.hpp"
#include "../cpu/scene.hpp"
#include "material.hpp"
#include "texture_bank.hpp"
#include "triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum class SceneFlags : uint32_t {
	eDefault,		// Fine for most formats
	eOneMaterialPerMesh,	// Required for albedo texturing, etc.
};

struct Scene {
	std::vector <int64_t> uuids;
	std::vector <TriangleMesh> meshes;
	std::vector <Material> materials;
	SceneFlags flags;

	static Scene from(core::DeviceResourceCollection &drc, const cpu::Scene &other) {

		Scene result;
		result.flags = SceneFlags::eDefault;
		result.uuids = other.uuids;
		
		for (auto &m : other.meshes) {
			// TODO: split the mesh for each material if needed
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