#include "gfx/vulkan/scene.hpp"

namespace jvl::gfx::vulkan {

Scene Scene::from(core::DeviceResourceCollection &drc, const cpu::Scene &other, SceneFlags flags)
{
	Scene result;
	result.flags = flags;
	
	for (size_t i = 0; i < other.meshes.size(); i++) {
		auto &m = other.meshes[i];

		int64_t original = other.mesh_to_object[m];

		if (flags == SceneFlags::eOneMaterialPerMesh && m.material_usage.size() > 1) {
			std::map <int, buffer <int3>> triangles;

			// TODO: put in the same device memory, but different ranges...
			size_t size = m.triangles.size();
			for (size_t i = 0; i < size; i++) {
				int mid = m.materials[i];
				triangles[mid].push_back(m.triangles[i]);
			}

			// fmt::println("# of triangles for each material...");
			// for (auto &[mid, tris] : triangles)
			// 	fmt::println("  {} -> {}", mid, tris.size());

			std::vector <core::TriangleMesh> split;
			for (auto &[mid, tris] : triangles) {
				auto tmesh = m;
				tmesh.triangles = tris;
				tmesh.material_usage.clear();
				tmesh.material_usage.insert(mid);

				auto vkm = TriangleMesh::from(drc.allocator(), tmesh).value();
				result.meshes.push_back(vkm);
				result.mesh_to_object.add(vkm, original);
			}
		} else {
			auto vkm = TriangleMesh::from(drc.allocator(), m).value();
			result.meshes.push_back(vkm);
			result.mesh_to_object.add(vkm, original);
		}
	}

	for (auto &m : other.materials)
		result.materials.push_back(m);

	return result;
}

} // namespace jvl::gfx::vulkan