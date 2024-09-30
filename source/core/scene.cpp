#include <array>
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include <fmt/printf.h>

#include "core/scene.hpp"
#include "core/material.hpp"
#include "core/messaging.hpp"
#include "engine/imported_asset.hpp"
#include "math_types.hpp"

namespace jvl::core {

void Scene::add(const Object &obj)
{
	objects[obj.id()] = obj;
}

void Scene::add_root(const Object &obj)
{
	int64_t global = obj.id();
	objects[global] = obj;
	root.insert(global);
}

void Scene::add(const engine::ImportedAsset &asset)
{
	Object top;
	top.uuid = new_uuid <Object> ();
	top.name = asset.path.stem();

	// Each geometry is its own object
	for (size_t i = 0; i < asset.geometries.size(); i++) {
		auto name = asset.names[i];
		auto geometry = asset.geometries[i];

		auto &mids = geometry.face_properties
			.at(Mesh::material_key)
			.as <buffer <int>> ();

		// Find the unique materials
		std::unordered_set <int> ref;
		for (auto &i : mids) {
			if (i >= 0)
				ref.insert(i);
		}

		// Generate list of materials and the reindex map
		std::unordered_map <int, int> reindex;

		std::vector <Material> materials;
		for (auto i : ref) {
			reindex[i] = materials.size();
			materials.push_back(asset.materials[i]);
		}

		for (auto &i : mids)
			i = reindex[i];

		// Add the object
		UUID uuid = new_uuid <Object> ();

		Object obj;
		obj.uuid = uuid;
		obj.name = name;
		obj.geometry = geometry;
		obj.materials = materials;

		add(obj);

		top.children.insert(uuid.global);
	}

	add_root(top);
}

Scene::Object &Scene::operator[](int64_t id)
{
	return objects[id];
}

const Scene::Object &Scene::operator[](int64_t id) const
{
	return objects.at(id);
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
void write_buffer(std::ofstream &fout, const buffer <T> &buf)
{
	write_int(fout, buf.size());
	fout.write((char *) buf.data(), buf.size() * sizeof(T));
}

void write_typed_buffer(std::ofstream &fout, const typed_buffer &buf)
{
	write_int(fout, buf.index());
	if (auto ib = buf.get <buffer <int>> ())
		return write_buffer(fout, ib.value());
	else if (auto ib2 = buf.get <buffer <int2>> ())
		return write_buffer(fout, ib2.value());
	else if (auto ib3 = buf.get <buffer <int3>> ())
		return write_buffer(fout, ib3.value());
	else if (auto ib4 = buf.get <buffer <int4>> ())
		return write_buffer(fout, ib4.value());
	else if (auto fb = buf.get <buffer <float>> ())
		return write_buffer(fout, fb.value());
	else if (auto fb2 = buf.get <buffer <float2>> ())
		return write_buffer(fout, fb2.value());
	else if (auto fb3 = buf.get <buffer <float3>> ())
		return write_buffer(fout, fb3.value());
	else if (auto fb4 = buf.get <buffer <float4>> ())
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

void write_material_property_value(std::ofstream &fout, const property_value &value)
{
	write_int(fout, value.index());
	if (auto opt_f = value.get <float> ()) {
		auto f = opt_f.value();
		fout.write((char *) &f, sizeof(f));
	} else if (auto opt_f2 = value.get <float2> ()) {
		auto f2 = opt_f2.value();
		fout.write((char *) &f2, sizeof(f2));
	} else if (auto opt_f3 = value.get <float3> ()) {
		auto f3 = opt_f3.value();
		fout.write((char *) &f3, sizeof(f3));
	} else if (auto opt_str = value.get <std::string> ()) {
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
