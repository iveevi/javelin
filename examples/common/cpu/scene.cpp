#include <core/logging.hpp>

#include "scene.hpp"

namespace jvl::cpu {

// TODO: generate the kernels for each material on demand
Scene Scene::from(const core::Scene &scene)
{
	Scene result;

	// TODO: filter through all objects with geometry
	for (auto &[id, _] : scene.mapping) {
		auto &obj = scene[id];

		if (obj.has_geometry()) {
			auto g = obj.get_geometry();

			auto &mids = g.face_properties
					.at(ImportedMesh::material_key)
					.as <std::vector <int>> ();

			size_t offset = result.materials.size();
			for (auto &i : mids)
				i += offset;

			auto tmesh = core::TriangleMesh::from(g).value();
			result.meshes.push_back(tmesh);

			result.mesh_to_object.add(tmesh, obj);
		}

		for (auto &i : obj.materials)
			result.materials.push_back(obj.get_material(i));
	}

	return result;
}

} // namespace jvl::cpu
