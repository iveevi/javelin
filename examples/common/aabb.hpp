#pragma once

#include "ray.hpp"

namespace jvl::core {

struct AABB {
	glm::vec3 min { 0, 0, 0 };
	glm::vec3 max { 0, 0, 0 };

	glm::vec3 center() const {
		return glm::vec3 {
			(min.x + max.x) / 2,
			(min.y + max.y) / 2,
			(min.z + max.z) / 2
		};
	}

	float surface_area() const {
		glm::vec3 d = max - min;
		return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
	}

	bool intersects(const Ray &, float, float) const;

	static AABB from_sphere(const glm::vec3 &center, float radius) {
		return AABB {
			center - glm::vec3 { radius, radius, radius },
			center + glm::vec3 { radius, radius, radius }
		};
	}

	static AABB from_triangle(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) {
		return AABB {
			glm::vec3 {
				std::min(p0.x, std::min(p1.x, p2.x)),
				std::min(p0.y, std::min(p1.y, p2.y)),
				std::min(p0.z, std::min(p1.z, p2.z))
			},
			glm::vec3 {
				std::max(p0.x, std::max(p1.x, p2.x)),
				std::max(p0.y, std::max(p1.y, p2.y)),
				std::max(p0.z, std::max(p1.z, p2.z))
			}
		};
	}

	static AABB combine(const AABB &, const AABB &);
};

} // namespace jvl::core
