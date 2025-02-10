#pragma once

#include "ray.hpp"

namespace jvl::gfx::cpu {

struct Intersection {
	float time;
	glm::vec3 normal;
	glm::vec3 point;
	glm::vec3 barycentric;
	bool backfacing;
	int material;

	// TODO: handle if backfacing
	glm::vec3 offset() const {
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
				       const glm::vec3 &,
				       const glm::vec3 &,
				       const glm::vec3 &);

} // namespace jvl::gfx::cpu
