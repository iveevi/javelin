#pragma once

#include "../../math_types.hpp"
#include "../../core/ray.hpp"

namespace jvl::gfx::cpu {

struct Intersection {
	float time;
	float3 normal;
	float3 point;
	float3 barycentric;
	bool backfacing;
	int material;

	// TODO: handle if backfacing
	float3 offset() const {
		static constexpr float epsilon = 1e-3f;
		return point + epsilon * normal;
	}

	void update(const Intersection &other) {
		if (other.time < 0)
			return;

		if (time < 0 || time > other.time) {
			time = other.time;
			normal = other.normal;
			point = other.point;
			barycentric = other.barycentric;
			backfacing = other.backfacing;
			material = other.material;
		}
	}

	static Intersection miss() {
		Intersection sh;
		sh.time = -1;
		return sh;
	}
};

Intersection ray_triangle_intersection(const core::Ray &,
				       const float3 &,
				       const float3 &,
				       const float3 &);

} // namespace jvl::gfx::cpu
