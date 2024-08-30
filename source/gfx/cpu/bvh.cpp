#include <algorithm>
#include <random>

#include <fmt/printf.h>

#include "gfx/cpu/bvh.hpp"

namespace jvl::gfx::cpu {

// TODO: tmeplate parameter...
int64_t build_bvh(BVH &destination, const std::vector <BVHNode> &source, const core::AABB &combined, BuildMode mode)
{
	static std::mt19937 rng;
	static std::uniform_int_distribution <int> axis_dist(0, 2);

	if (source.size() == 0) {
		fmt::println("no nodes given, failed to generate BVH structure");
		abort();
	} else if (source.size() == 1) {
		destination.push_back(source[0]);
		return 1;
	} else if (source.size() == 2) {
		BVHNode node;
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
	std::vector <BVHNode> left_source;
	std::vector <BVHNode> right_source;

	core::AABB left_aabb;
	core::AABB right_aabb;

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
					left_aabb = core::AABB::combine(left_aabb, source[centers[i].second].aabb);
			} else {
				right_source.push_back(source[centers[i].second]);

				if (right_source.size() == 1)
					right_aabb = source[centers[i].second].aabb;
				else
					right_aabb = core::AABB::combine(right_aabb, source[centers[i].second].aabb);
			}
		}
	} else {
		static constexpr int buckets = 20;

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

		// TODO: thread pool over this if there are too many buckets...
		for (int i = 0; i <= buckets; i++) {
			float split = extent_min + extent * i / buckets;

			core::AABB left_aabb;
			core::AABB right_aabb;

			int left_count = 0;
			int right_count = 0;

			for (int j = 0; j < source.size(); j++) {
				if (source[j].aabb.center()[axis] < split) {
					left_count++;
					if (left_count == 1)
						left_aabb = source[j].aabb;
					else
						left_aabb = core::AABB::combine(left_aabb, source[j].aabb);
				} else {
					right_count++;
					if (right_count == 1)
						right_aabb = source[j].aabb;
					else
						right_aabb = core::AABB::combine(right_aabb, source[j].aabb);
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
					left_aabb = core::AABB::combine(left_aabb, source[i].aabb);
			} else {
				right_source.push_back(source[i]);

				if (right_source.size() == 1)
					right_aabb = source[i].aabb;
				else
					right_aabb = core::AABB::combine(right_aabb, source[i].aabb);
			}
		}

		if (left_source.size() == 0 || right_source.size() == 0) {
			// Resort to longest axis regular split
			return build_bvh(destination, source, combined, eLongestAxis);
		}
	}

	// Create new node
	int64_t index = destination.size();

	BVHNode node;
	int64_t left_size = build_bvh(destination, left_source, left_aabb, mode);
	int64_t right_size = build_bvh(destination, right_source, right_aabb, mode);

	node.aabb = combined;
	// TODO: left is always +/- 1
	node.left = 1;
	node.right = 1 + left_size;
	destination.insert(destination.begin() + index, node);

	return 1 + left_size + right_size;
}

} // namespace jvl::gfx::cpu
