#pragma once

#include <littlevk/littlevk.hpp>

#include "../../core/device_resource_collection.hpp"
#include "../cpu/scene.hpp"
#include "material.hpp"
#include "triangle_mesh.hpp"

namespace jvl::gfx::vulkan {

enum class SceneFlags : uint32_t {
	eDefault,		// Fine for most formats
	eOneMaterialPerMesh,	// Required for albedo texturing, etc.
};

struct Scene {
	std::vector <TriangleMesh> meshes;
	std::vector <Material> materials;
	SceneFlags flags;

	static Scene from(core::DeviceResourceCollection &drc, const cpu::Scene &other, SceneFlags flags) {
		Scene result;
		result.flags = flags;
		
		for (size_t i = 0; i < other.meshes.size(); i++) {
			// TODO: put uuid in core mesh (unique)
			auto &m = other.meshes[i];
			if (flags == SceneFlags::eOneMaterialPerMesh && m.material_usage.size() > 1) {
				std::map <int, buffer <int3>> triangles;

				// TODO: put in the same device memory, but different ranges...
				size_t size = m.triangles.size();
				for (size_t i = 0; i < size; i++) {
					int mid = m.materials[i];
					triangles[mid].push_back(m.triangles[i]);
				}

				fmt::println("# of triangles for each material...");
				for (auto &[mid, tris] : triangles)
					fmt::println("  {} -> {}", mid, tris.size());

				std::vector <core::TriangleMesh> split;
				for (auto &[mid, tris] : triangles) {
					auto tmesh = m;
					tmesh.triangles = tris;
					tmesh.material_usage.clear();
					tmesh.material_usage.insert(mid);

					auto vkm = TriangleMesh::from(drc.allocator(), tmesh).value();
					result.meshes.push_back(vkm);
				}
			} else {
				// TODO: split the mesh for each material if needed
				auto vkm = TriangleMesh::from(drc.allocator(), m).value();
				result.meshes.push_back(vkm);
			}
		}

		for (auto &m : other.materials) {
			auto vkm = Material::from(drc, m).value();
			result.materials.push_back(vkm);
		}

		return result;
	}
};

} // namespace jvl::gfx::vulkan