#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <filesystem>

#include "types.hpp"

namespace jvl {

struct Mesh {
	// Vertex property keys
	static constexpr char position_key[] = "position";
	static constexpr char normal_key[] = "normal";
	static constexpr char uv_key[] = "uv";

	// Face property keys
	static constexpr char triangle_key[] = "triangles";
	static constexpr char quadrilateral_key[] = "quadrilaterals";

	size_t vertex_count;
	property <typed_buffer> vertex_properties;
	property <typed_buffer> face_properties;
};

struct Kernel {
	enum {
		eCPU,
		eCUDA,
		eVulkan,
		eOpenGL
	} backend;
};

// struct Material {};

struct TriangleMesh {
	buffer <float3> positions;
	buffer <int3> triangles;

	// TODO: additional vertex properties properties

	static std::optional <TriangleMesh> from(const Mesh &m) {
		TriangleMesh tm;

		if (auto opt_pos = m.vertex_properties
				.maybe_at(Mesh::position_key)
				.as <buffer <float3>> ())
			tm.positions = opt_pos.value();
		else
			return std::nullopt;

		if (auto opt_tris = m.face_properties
				.maybe_at(Mesh::triangle_key)
				.as <buffer <int3>> ())
			tm.triangles = opt_tris.value();

		// TODO: quadrilaterals -- append

		if (tm.triangles.empty())
			return std::nullopt;

		return tm;
	}
};

// Constructed when loading a scene from a path
struct Preset {
	std::vector <Mesh> geometry;
	std::filesystem::path path;

	// TODO: materials as well

	static std::optional <Preset> from(const std::filesystem::path &);
};

}

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <fmt/std.h>
#include <fmt/printf.h>

template <>
struct std::hash <tinyobj::index_t>
{
	size_t operator()(const tinyobj::index_t &k) const {
		auto h = hash <int> ();
		return ((h(k.vertex_index) ^ (h(k.normal_index) << 1)) >> 1) ^ (h(k.texcoord_index) << 1);
	}
};

namespace tinyobj {

inline bool operator==(const tinyobj::index_t &a, const tinyobj::index_t &b)
{
	return std::tie(a.vertex_index, a.normal_index, a.texcoord_index)
		== std::tie(b.vertex_index, b.normal_index, b.texcoord_index);
}

}

namespace jvl {

std::optional <Preset> Preset::from(const std::filesystem::path &path)
{
	Preset preset;
	preset.path = path;

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig reader_config;

	reader_config.mtl_search_path = path.parent_path();

	if (!reader.ParseFromFile(path, reader_config)) {
		if (!reader.Error().empty())
			fmt::println("tinyobj-error: {}", reader.Error());

		exit(1);
	}

	if (!reader.Warning().empty())
		fmt::println("tinyobj-warning: {}", reader.Warning());

	auto &attrib = reader.GetAttrib();
	auto &shapes = reader.GetShapes();
	auto &materials = reader.GetMaterials();

	for (size_t s = 0; s < shapes.size(); s++) {
		fmt::println("SHAPE>>>");

		buffer <float3> positions;
		buffer <float3> normals;
		buffer <float2> uvs;

		buffer <int3> triangles;
		buffer <int4> quadrilaterals;

		std::unordered_map <float3, int32_t> position_map;
		std::unordered_map <tinyobj::index_t, int32_t> index_map;

		// tinyobj::shape_t;

		// Loop over faces (polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Triangles or quadrilaterals
			int32_t indices[4] { -1, -1, -1, -1 };

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				if (index_map.count(idx)) {
					indices[v] = index_map[idx];
					continue;
				}

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

				float3 p { vx, vy, vz };

				if (position_map.count(p)) {
					indices[v] = position_map[p];
					continue;
				} else {
					indices[v] = positions.size();
				}

				positions.push_back(p);

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index)+0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index)+1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index)+2];
					normals.push_back({ nx, ny, nz });
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index)+0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index)+1];
					uvs.push_back({ tx, ty });
				}
			}

			if (indices[3] == -1) {
				int3 f { indices[0], indices[1], indices[2] };
				triangles.push_back(f);
			} else {
				int4 f { indices[0], indices[1], indices[2], indices[3] };
				quadrilaterals.push_back(f);
			}

			index_offset += fv;
		}

		fmt::println("size: {} vertices, {} normals, {} uvs", positions.size(), normals.size(), uvs.size());
		fmt::println("size: {} tris, {} quads", triangles.size(), quadrilaterals.size());

		Mesh mesh;

		mesh.vertex_count = positions.size();

		if (positions.size())
			mesh.vertex_properties[Mesh::position_key] = positions;
		if (normals.size())
			mesh.vertex_properties[Mesh::normal_key] = normals;
		if (uvs.size())
			mesh.vertex_properties[Mesh::uv_key] = uvs;

		if (triangles.size())
			mesh.face_properties[Mesh::triangle_key] = triangles;
		if (quadrilaterals.size())
			mesh.face_properties[Mesh::quadrilateral_key] = quadrilaterals;

		auto tmesh = TriangleMesh::from(mesh).value();
		fmt::println("triangle mesh: {} vertices, {} faces", tmesh.positions.size(), tmesh.triangles.size());
	}

	return preset;
}

}

#include "transform.hpp"

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::filesystem::path path = argv[1];
	fmt::println("path to scene: {}", path);

	jvl::Preset::from(path);
}
