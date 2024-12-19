#include "aabb.hpp"

namespace jvl::core {

AABB AABB::combine(const AABB &a, const AABB &b)
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

bool AABB::intersects(const Ray &ray, float tmin, float tmax) const
{
	for (int i = 0; i < 3; i++) {
		float invD = 1.0 / ray.direction[i];
		float t0 = (min[i] - ray.origin[i]) * invD;
		float t1 = (max[i] - ray.origin[i]) * invD;

		if (invD < 0.0)
			std::swap(t0, t1);

		tmin = t0 > tmin ? t0 : tmin;
		tmax = t1 < tmax ? t1 : tmax;
		if (tmax < tmin)
			return false;
	}

	return true;
}

} // namespace jvl::core
