#pragma once

#include <vector>

#include "../../core/aabb.hpp"
#include "../../math_types.hpp"
#include "intersection.hpp"

namespace jvl::gfx::cpu {

// TODO: threaded binary tree
// TODO: profile and measure times on a scene...
struct BVHNode {
	core::AABB aabb;
	int32_t left = 0;
	int32_t right = 0;
	int32_t mesh = 0;
	int32_t index = 0;
};

struct BVH : std::vector <BVHNode> {
	using std::vector <BVHNode> ::vector;
};

enum BuildMode {
	eRandomAxis,
	eLongestAxis,
	eSurfaceAreaHeuristic
};

int64_t build_bvh(BVH &, const std::vector <BVHNode> &, const core::AABB &, BuildMode);

} // namespace jvl::gfx::cpu
