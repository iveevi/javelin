#include <stack>

#include "gfx/cpu/scene.hpp"
#include "engine/imported_asset.hpp"

namespace jvl::gfx::cpu {

void Scene::add(const engine::ImportedAsset &imported_asset)
{
	// Add the materials first, then the geometry (to offset)
	size_t offset = materials.size();
	for (auto &m : imported_asset.materials) {
		// TODO: generate the kernels for each material
		// and then compile on demand
		materials.push_back(m);
	}

	for (auto &g : imported_asset.geometries) {
		auto tmesh = core::TriangleMesh::from(g).value();
		for (auto &i : tmesh.materials)
			i += offset;

		meshes.push_back(tmesh);
	}
}

void Scene::build_bvh()
{
	std::vector <BVHNode> nodes;
	core::AABB combined;
	for (size_t i = 0; i < meshes.size(); i++) {
		auto &m = meshes[i];
		for (size_t j = 0; j < m.triangles.size(); j++) {
			auto &tri = m.triangles[j];

			auto v0 = m.positions[tri.x];
			auto v1 = m.positions[tri.y];
			auto v2 = m.positions[tri.z];
			auto aabb = core::AABB::from_triangle(v0, v1, v2);

			combined = core::AABB::combine(combined, aabb);

			cpu::BVHNode node;
			node.aabb = aabb;
			node.mesh = i;
			node.index = j;

			nodes.push_back(node);
		}
	}

	bvh.clear();

	cpu::build_bvh(bvh, nodes, combined, cpu::eSurfaceAreaHeuristic);
}

cpu::Intersection cpu::Scene::trace(const core::Ray &ray)
{
	auto sh = cpu::Intersection::miss();

	std::stack <int64_t> stack;
	stack.push(0);

	while (!stack.empty()) {
		int64_t index = stack.top();
		stack.pop();

		const cpu::BVHNode &node = bvh[index];

		int64_t left = node.left;
		int64_t right = node.right;

		bool hit_aabb_ = node.aabb.intersects(ray, 0, sh.time < 0 ? 1e16 : sh.time);
		if (!hit_aabb_)
			continue;

		if (left == 0 && right == 0) {
			// Leaf node
			auto &m = meshes[node.mesh];
			auto &tri = m.triangles[node.index];
			auto v0 = m.positions[tri.x];
			auto v1 = m.positions[tri.y];
			auto v2 = m.positions[tri.z];

			auto hit = gfx::cpu::ray_triangle_intersection(ray, v0, v1, v2);

			// TODO: more attributes (normal, uv...)
			hit.material = m.materials[node.index];

			sh.update(hit);
		}

		if (left != 0)
			stack.push(index + left);
		if (right != 0)
			stack.push(index + right);
	}

	return sh;
}

} // namespace jvl::gfx::cpu
