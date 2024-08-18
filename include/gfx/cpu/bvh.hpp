#pragma once

#include <vector>

#include "../../math_types.hpp"

namespace jvl::gfx::cpu {

struct AABB {
	float3 min { 0, 0, 0 };
	float3 max { 0, 0, 0 };

	float3 center() const {
		return float3 {
			(min.x + max.x) / 2,
			(min.y + max.y) / 2,
			(min.z + max.z) / 2
		};
	}

	float surface_area() const {
		float3 d = max - min;
		return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
	}

	static AABB from_sphere(const float3 &center, float radius) {
		return AABB {
			center - float3 { radius, radius, radius },
			center + float3 { radius, radius, radius }
		};
	}

	static AABB from_triangle(const float3 &p0, const float3 &p1, const float3 &p2) {
		return AABB {
			float3 {
				std::min(p0.x, std::min(p1.x, p2.x)),
				std::min(p0.y, std::min(p1.y, p2.y)),
				std::min(p0.z, std::min(p1.z, p2.z))
			},
			float3 {
				std::max(p0.x, std::max(p1.x, p2.x)),
				std::max(p0.y, std::max(p1.y, p2.y)),
				std::max(p0.z, std::max(p1.z, p2.z))
			}
		};
	}
};

AABB combine(const AABB &, const AABB &);
bool hit_aabb(const float3 &, const float3 &, const AABB &, float, float);

// TODO: threaded binary tree
struct BVHCompact {
	AABB aabb;
	int32_t left = 0;
	int32_t right = 0;
	int32_t mesh = 0;
	int32_t index = 0;
};

enum BuildMode {
	eRandomAxis,
	eLongestAxis,
	eSurfaceAreaHeuristic
};

int64_t make_acceleration_structure(std::vector <BVHCompact> &, const std::vector <BVHCompact> &, const AABB &, BuildMode);

enum TraceMode {
	eNone,
	eClosestFirst
};

} // namespace jvl::gfx::cpu
