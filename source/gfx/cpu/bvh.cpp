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
		node.left = 1;
		node.right = 2;

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
	// } else {
	// 	static constexpr int buckets = 100;
	//
	// 	// Longest axis
	// 	int axis = 0;
	// 	float max_length = 0;
	// 	for (int i = 0; i < 3; i++) {
	// 		float length = combined.max[i] - combined.min[i];
	// 		if (length > max_length) {
	// 		max_length = length;
	// 		axis = i;
	// 		}
	// 	}
	//
	// 	float extent_min = combined.min[axis];
	// 	float extent = combined.max[axis] - combined.min[axis];
	//
	// 	// Iterate over each bucket
	// 	float min_cost = std::numeric_limits <float>::max();
	// 	float min_split = 0;
	//
	// 	for (int i = 0; i <= buckets; i++) {
	// 		float split = extent_min + extent * i / buckets;
	//
	// 		AABB left_aabb;
	// 		AABB right_aabb;
	//
	// 		int left_count = 0;
	// 		int right_count = 0;
	//
	// 		for (int j = 0; j < source.size(); j++) {
	// 			if (source[j].aabb.center()[axis] < split) {
	// 				left_count++;
	// 				if (left_count == 1)
	// 					left_aabb = source[j].aabb;
	// 				else
	// 					left_aabb = combine(left_aabb, source[j].aabb);
	// 			} else {
	// 				right_count++;
	// 				if (right_count == 1)
	// 					right_aabb = source[j].aabb;
	// 				else
	// 					right_aabb = combine(right_aabb, source[j].aabb);
	// 			}
	// 		}
	//
	// 		float left_cost = left_count * left_aabb.surface_area();
	// 		float right_cost = right_count * right_aabb.surface_area();
	// 		float cost = 1 + (left_cost + right_cost)/combined.surface_area();
	//
	// 		if (cost < min_cost) {
	// 			min_cost = cost;
	// 			min_split = split;
	// 		}
	// 	}
	//
	// 	// Split source into two groups
	// 	for (int i = 0; i < source.size(); i++) {
	// 		if (source[i].aabb.center()[axis] < min_split) {
	// 			left_source.push_back(source[i]);
	//
	// 			if (left_source.size() == 1)
	// 				left_aabb = source[i].aabb;
	// 			else
	// 				left_aabb = combine(left_aabb, source[i].aabb);
	// 		} else {
	// 			right_source.push_back(source[i]);
	//
	// 			if (right_source.size() == 1)
	// 				right_aabb = source[i].aabb;
	// 			else
	// 				right_aabb = combine(right_aabb, source[i].aabb);
	// 		}
	// 	}
	//
	// 	if (left_source.size() == 0 || right_source.size() == 0) {
	// 		// Resort to longest axis regular split
	// 		return make_acceleration_structure(destination, source, combined, eLongestAxis);
	// 	}
	}

	// Create new node
	int64_t index = destination.size();

	BVHCompact node;
	int64_t left_size = make_acceleration_structure(destination, left_source, left_aabb, mode);
	int64_t right_size = make_acceleration_structure(destination, right_source, right_aabb, mode);

	node.aabb = combined;
	// TODO: left is always +/- 1
	node.left = 1;
	node.right = 1 + left_size;
	destination.insert(destination.begin() + index, node);

	return 1 + left_size + right_size;
}

} // namespace jvl::gfx::cpu
