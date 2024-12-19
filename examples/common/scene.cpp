#include <array>
#include <cassert>
#include <fstream>

#include <fmt/printf.h>

#include <core/logging.hpp>
#include <core/math.hpp>

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

void Scene::add(const engine::ImportedAsset &asset)
{
	Object &top = add_root();
	top.name = asset.path.stem();

	// Each geometry is its own object
	for (size_t i = 0; i < asset.geometries.size(); i++) {
		auto name = asset.names[i];
		auto geometry = asset.geometries[i];

		auto &mids = geometry.face_properties
			.at(Mesh::material_key)
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

// Serializing to a file
static constexpr std::array <char, 4> JVL_SCENE_FOURCC { 'J', 'V', 'L', 'S' };

void write_int(std::ofstream &fout, int32_t x)
{
	fout.write((char *) &x, sizeof(x));
}

void write_string(std::ofstream &fout, const std::string &s)
{
	write_int(fout, s.size());
	fout.write(s.data(), s.size());
}

template <typename T>
void write_buffer(std::ofstream &fout, const std::vector <T> &buf)
{
	write_int(fout, buf.size());
	fout.write((char *) buf.data(), buf.size() * sizeof(T));
}

void write_typed_buffer(std::ofstream &fout, const typed_vector &buf)
{
	write_int(fout, buf.index());
	// TODO: visitor pattern...
	if (auto ib = buf.get <std::vector <int>> ())
		return write_buffer(fout, ib.value());
	else if (auto ib2 = buf.get <std::vector <int2>> ())
		return write_buffer(fout, ib2.value());
	else if (auto ib3 = buf.get <std::vector <int3>> ())
		return write_buffer(fout, ib3.value());
	else if (auto ib4 = buf.get <std::vector <int4>> ())
		return write_buffer(fout, ib4.value());
	else if (auto fb = buf.get <std::vector <float>> ())
		return write_buffer(fout, fb.value());
	else if (auto fb2 = buf.get <std::vector <float2>> ())
		return write_buffer(fout, fb2.value());
	else if (auto fb3 = buf.get <std::vector <float3>> ())
		return write_buffer(fout, fb3.value());
	else if (auto fb4 = buf.get <std::vector <float4>> ())
		return write_buffer(fout, fb4.value());
	else
		abort();
}

void write_mesh(std::ofstream &fout, const Mesh &mesh)
{
	write_int(fout, mesh.vertex_count);

	// TODO: optionally any compression

	write_int(fout, mesh.vertex_properties.size());
	for (const auto &[k, v] : mesh.vertex_properties) {
		write_string(fout, k);
		write_typed_buffer(fout, v);
	}

	write_int(fout, mesh.face_properties.size());
	for (const auto &[k, v] : mesh.face_properties) {
		write_string(fout, k);
		write_typed_buffer(fout, v);
	}
}

void write_material_property_value(std::ofstream &fout, const material_property &value)
{
	write_int(fout, value.index());
	if (auto opt_f = value.get <float> ()) {
		auto f = opt_f.value();
		fout.write((char *) &f, sizeof(f));
	} else if (auto opt_f3 = value.get <color3> ()) {
		auto f3 = opt_f3.value();
		fout.write((char *) &f3, sizeof(f3));
	} else if (auto opt_str = value.get <texture> ()) {
		auto str = opt_str.value();
		write_string(fout, str);
	} else {
		fmt::println("{} failed due to unsupported type @{}", __FUNCTION__, value.index());
		abort();
	}
}

void write_material(std::ofstream &fout, const Material &material)
{
	write_int(fout, material.values.size());
	for (const auto &[k, v] : material.values) {
		write_string(fout, k);
		write_material_property_value(fout, v);
	}
}

void Scene::write(const std::filesystem::path &path)
{
	std::ofstream fout(path);
	assert(fout);

	// Header for a compatibility check
	// TODO: add version as well...
	fout.write(JVL_SCENE_FOURCC.data(), JVL_SCENE_FOURCC.size());

	// write_int(fout, objects.size());
	// for (auto &obj : objects) {
	// 	write_int(fout, obj.geometry.has_value());
	// 	if (obj.geometry)
	// 		write_mesh(fout, obj.geometry.value());

	// 	write_int(fout, obj.materials.size());
	// 	for (auto &material : obj.materials)
	// 		write_material(fout, material);
	// }

	fout.close();
}

} // namespace jvl::core
