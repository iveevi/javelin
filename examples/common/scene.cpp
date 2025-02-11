#include <array>
#include <cassert>
#include <fstream>

#include <fmt/printf.h>

#include <core/logging.hpp>

#include "scene.hpp"
#include "material.hpp"
#include "imported_asset.hpp"

namespace jvl::core {

MODULE(core-scene);

Scene::Object &Scene::add()
{
	auto uuid = new_uuid <Object> ();
	mapping[uuid.global] = objects.size();
	objects.emplace_back(uuid, this);
	return objects.back();
}

Scene::Object &Scene::add_root()
{
	auto uuid = new_uuid <Object> ();
	mapping[uuid.global] = objects.size();
	root.insert(uuid.global);
	objects.emplace_back(uuid, this);
	return objects.back();
}

void Scene::add(const ImportedAsset &asset)
{
	Object &top = add_root();
	top.name = asset.path.stem();

	// Each geometry is its own object
	for (size_t i = 0; i < asset.geometries.size(); i++) {
		auto name = asset.names[i];
		auto geometry = asset.geometries[i];

		auto &mids = geometry.face_properties
			.at(ImportedMesh::material_key)
			.as <std::vector <int32_t>> ();

		// Find the unique materials
		std::set <int> ref;
		for (auto &i : mids) {
			if (i >= 0)
				ref.insert(i);
		}

		JVL_ASSERT_PLAIN(ref.size() > 0);

		// Generate list of materials and the reindex map
		std::map <int, int> reindex;

		std::vector <int64_t> local_materials;
		for (auto i : ref) {
			int64_t index = materials.size();
			reindex[i] = local_materials.size();
			local_materials.push_back(index);
			materials.add(asset.materials[i]);
		}

		for (auto &i : mids)
			i = reindex[i];

		// Add the object
		Object &obj = add();
		obj.name = name;
		obj.geometry = geometries.size();
		obj.materials = local_materials;

		top.children.insert(obj.id());

		geometries.emplace_back(geometry);
	}
}

Scene::Object &Scene::operator[](int64_t id)
{
	int64_t index = mapping[id];
	auto it = objects.begin();
	while (index-- > 0)
		it++;

	return *it;
}

const Scene::Object &Scene::operator[](int64_t id) const
{
	int64_t index = mapping.at(id);
	auto it = objects.begin();
	while (index-- > 0)
		it++;

	return *it;
}

} // namespace jvl::core
