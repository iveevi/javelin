#include <algorithm>
#include <random>

#include "gfx/cpu/bvh.hpp"

namespace jvl::gfx::cpu {

AABB combine(const AABB &a, const AABB &b)
{
	return AABB {
		float3 {
			std::min(a.min.x, b.min.x),
			std::min(a.min.y, b.min.y),
			std::min(a.min.z, b.min.z)
		},
		float3 {
			std::max(a.max.x, b.max.x),
			std::max(a.max.y, b.max.y),
			std::max(a.max.z, b.max.z)
		}
	};
}

bool hit_aabb(const float3 &ray, const float3 &origin, const AABB &aabb, float tmin, float tmax)
{
	for (int i = 0; i < 3; i++) {
		float invD = 1.0 / ray[i];
		float t0 = (aabb.min[i] - origin[i]) * invD;
		float t1 = (aabb.max[i] - origin[i]) * invD;

		if (invD < 0.0)
			std::swap(t0, t1);

		tmin = t0 > tmin ? t0 : tmin;
		tmax = t1 < tmax ? t1 : tmax;
		if (tmax < tmin)
			return false;
	}

	return true;
}

int64_t make_acceleration_structure(std::vector <BVHCompact> &destination, const std::vector <BVHCompact> &source, const AABB &combined, BuildMode mode)
{
	static std::mt19937 rng;
	static std::uniform_int_distribution <int> axis_dist(0, 2);

	if (source.size() == 1) {
		destination.push_back(source[0]);
		return 1;
	}

	if (source.size() == 2) {
		BVHCompact node;
		node.aabb = combined;
		node.branches = (1ul << 32) | 2;

		destination.push_back(node);
		destination.push_back(source[0]);
		destination.push_back(source[1]);

		return 3;
	}

	// Choose  axis
	int axis = 0;
	if (mode == eRandomAxis) {
		axis = axis_dist(rng);
	} else if (mode == eLongestAxis) {
		float max_length = 0;
		for (int i = 0; i < 3; i++) {
			float length = combined.max[i] - combined.min[i];
			if (length > max_length) {
			max_length = length;
			axis = i;
			}
		}
	}

	// Final partition
	std::vector <BVHCompact> left_source;
	std::vector <BVHCompact> right_source;

	AABB left_aabb;
	AABB right_aabb;

	if (mode != eSurfaceAreaHeuristic) {
		// Sort source by center of AABB
		std::vector <std::pair <float, uint64_t>> centers;
		for (int i = 0; i < source.size(); i++)
			centers.push_back({ source[i].aabb.center()[axis], i });

		std::sort(centers.begin(), centers.end());

		// Split source into two groups
		for (int i = 0; i < centers.size(); i++) {
			if (i < centers.size() / 2) {
			left_source.push_back(source[centers[i].second]);

			if (left_source.size() == 1)
				left_aabb = source[centers[i].second].aabb;
			else
				left_aabb = combine(left_aabb, source[centers[i].second].aabb);
			} else {
			right_source.push_back(source[centers[i].second]);

			if (right_source.size() == 1)
				right_aabb = source[centers[i].second].aabb;
			else
				right_aabb = combine(right_aabb, source[centers[i].second].aabb);
			}
		}
	} else {
		static constexpr int buckets = 100;

		// Longest axis
		int axis = 0;
		float max_length = 0;
		for (int i = 0; i < 3; i++) {
			float length = combined.max[i] - combined.min[i];
			if (length > max_length) {
			max_length = length;
			axis = i;
			}
		}

		float extent_min = combined.min[axis];
		float extent = combined.max[axis] - combined.min[axis];

		// Iterate over each bucket
		float min_cost = std::numeric_limits <float>::max();
		float min_split = 0;

		for (int i = 0; i <= buckets; i++) {
			float split = extent_min + extent * i / buckets;

			AABB left_aabb;
			AABB right_aabb;

			int left_count = 0;
			int right_count = 0;

			for (int j = 0; j < source.size(); j++) {
				if (source[j].aabb.center()[axis] < split) {
					left_count++;
					if (left_count == 1)
						left_aabb = source[j].aabb;
					else
						left_aabb = combine(left_aabb, source[j].aabb);
				} else {
					right_count++;
					if (right_count == 1)
						right_aabb = source[j].aabb;
					else
						right_aabb = combine(right_aabb, source[j].aabb);
				}
			}

			float left_cost = left_count * left_aabb.surface_area();
			float right_cost = right_count * right_aabb.surface_area();
			float cost = 1 + (left_cost + right_cost)/combined.surface_area();

			if (cost < min_cost) {
				min_cost = cost;
				min_split = split;
			}
		}

		// Split source into two groups
		for (int i = 0; i < source.size(); i++) {
			if (source[i].aabb.center()[axis] < min_split) {
				left_source.push_back(source[i]);

				if (left_source.size() == 1)
					left_aabb = source[i].aabb;
				else
					left_aabb = combine(left_aabb, source[i].aabb);
			} else {
				right_source.push_back(source[i]);

				if (right_source.size() == 1)
					right_aabb = source[i].aabb;
				else
					right_aabb = combine(right_aabb, source[i].aabb);
			}
		}

		if (left_source.size() == 0 || right_source.size() == 0) {
			// Resort to longest axis regular split
			return make_acceleration_structure(destination, source, combined, eLongestAxis);
		}
	}

	// Create new node
	int64_t index = destination.size();

	BVHCompact node;
	int64_t left_size = make_acceleration_structure(destination, left_source, left_aabb, mode);
	int64_t right_size = make_acceleration_structure(destination, right_source, right_aabb, mode);

	node.aabb = combined;
	node.branches = (1ul << 32) | ((1 + left_size) & 0xFFFFFFFF);
	destination.insert(destination.begin() + index, node);

	return 1 + left_size + right_size;
}

// SurfaceHit trace_accelerated(const ParsedScene &scene, const std::vector <BVHCompact> &bvh, const Vector3 &ray, const Vector3 &origin, TraceMode options)
// {
//     SurfaceHit sh = SurfaceHit::miss();

//     std::stack <int64_t> stack;
//     stack.push(0);

//     while (!stack.empty()) {
//         int64_t index = stack.top();
//         stack.pop();

//         const BVHCompact *node = &bvh[index];
//         bool hit_aabb_ = hit_aabb(ray, origin, node->aabb, 0, sh.t < 0 ? 1e16 : sh.t);
//         if (!hit_aabb_)
//             continue;

//         int64_t left = node->branches >> 32;
//         int64_t right = node->branches & 0xFFFFFFFF;

//         if (left == 0 && right == 0) {
//             int64_t shape_index = node->index >> 32;
//             int64_t mesh_index = node->index & 0xFFFFFFFF;

//             ParsedShape shape = scene.shapes[shape_index];

//             if (std::holds_alternative <ParsedSphere> (shape)) {
//                 ParsedSphere sphere = std::get <ParsedSphere> (shape);

//                 SurfaceHit hit = hit_sphere(ray, origin,
//                     Sphere { sphere.position, sphere.radius });

//                 if (hit.t > 0 && (hit.t < sh.t || sh.t < 0)) {
//                     sh = hit;
//                     sh.id = sphere.material_id;
//                     sh.shape_id = shape_index;
//                     sh.triangle_id = -1;
                    
//                     if (sphere.area_light_id >= 0) {
//                         ParsedLight light = scene.lights[sphere.area_light_id];
//                         if (!std::holds_alternative <ParsedDiffuseAreaLight> (light))
//                             throw std::runtime_error("Unsupported light type");

//                         ParsedDiffuseAreaLight diffuse_light = std::get <ParsedDiffuseAreaLight> (light);
//                         sh.e = diffuse_light.radiance;
//                     }
//                 }
//             } else if (std::holds_alternative <ParsedTriangleMesh> (shape)) {
//                 ParsedTriangleMesh mesh = std::get <ParsedTriangleMesh> (shape);

//                 Triangle triangle;

//                 triangle.v0 = mesh.positions[mesh.indices[mesh_index][0]];
//                 triangle.v1 = mesh.positions[mesh.indices[mesh_index][1]];
//                 triangle.v2 = mesh.positions[mesh.indices[mesh_index][2]];

//                 triangle.has_normals = mesh.normals.size() > 0;
//                 if (triangle.has_normals) {
//                     triangle.n0 = mesh.normals[mesh.indices[mesh_index][0]];
//                     triangle.n1 = mesh.normals[mesh.indices[mesh_index][1]];
//                     triangle.n2 = mesh.normals[mesh.indices[mesh_index][2]];
//                 }

//                 triangle.has_uvs = mesh.uvs.size() > 0;
//                 if (triangle.has_uvs) {
//                     triangle.uv0 = mesh.uvs[mesh.indices[mesh_index][0]];
//                     triangle.uv1 = mesh.uvs[mesh.indices[mesh_index][1]];
//                     triangle.uv2 = mesh.uvs[mesh.indices[mesh_index][2]];
//                 }

//                 SurfaceHit hit = hit_triangle(ray, origin, triangle);
//                 if (hit.t > 0 && (hit.t < sh.t || sh.t < 0)) {
//                     sh = hit;
//                     sh.id = mesh.material_id;
//                     sh.shape_id = shape_index;
//                     sh.triangle_id = mesh_index;

//                     if (mesh.area_light_id >= 0) {
//                         ParsedLight light = scene.lights[mesh.area_light_id];
//                         if (!std::holds_alternative <ParsedDiffuseAreaLight> (light))
//                             throw std::runtime_error("Unsupported light type");

//                         ParsedDiffuseAreaLight diffuse_light = std::get <ParsedDiffuseAreaLight> (light);
//                         sh.e = diffuse_light.radiance;
//                     }
//                 }
//             }
//         }

//         if (options == eClosestFirst) {
//             if (left != 0 && right != 0) {
//                 const BVHCompact *left_node = &bvh[index + left];
//                 const BVHCompact *right_node = &bvh[index + right];

//                 float left_distance = length(origin - left_node->aabb.center());
//                 float right_distance = length(origin - right_node->aabb.center());

//                 if (left_distance < right_distance) {
//                     stack.push(index + right);
//                     stack.push(index + left);
//                 } else {
//                     stack.push(index + left);
//                     stack.push(index + right);
//                 }
//             } else if (left != 0) {
//                 stack.push(index + left);
//             } else if (right != 0) {
//                 stack.push(index + right);
//             }
//         } else {
//             if (left != 0)
//                 stack.push(index + left);
//             if (right != 0)
//                 stack.push(index + right);
//         }
//     }
    
//     return sh;
// }

} // namespace jvl::gfx::cpu