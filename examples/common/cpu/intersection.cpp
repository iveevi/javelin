#include "intersection.hpp"

namespace jvl::gfx::cpu {

Intersection ray_triangle_intersection(const core::Ray &ray,
				       const float3 &v0,
				       const float3 &v1,
				       const float3 &v2)
{
	Intersection sh = Intersection::miss();

	float3 e1 = v1 - v0;
	float3 e2 = v2 - v0;
	float3 p = cross(ray.direction, e2);

	float a = dot(e1, p);
	if (std::abs(a) < 1e-6)
		return sh;

	float f = 1.0 / a;
	float3 s = ray.origin - v0;
	float u = f * dot(s, p);

	if (u < 0.0 || u > 1.0)
		return sh;

	float3 q = cross(s, e1);
	float v = f * dot(ray.direction, q);

	if (v < 0.0 || u + v > 1.0)
		return sh;

	float t = f * dot(e2, q);
	if (t > 0.00001) {
		sh.time = t;
		sh.point = ray.origin + t * ray.direction;
		sh.barycentric = { 1 - u - v, u, v };
		sh.normal = normalize(cross(e1, e2));
		if (dot(sh.normal, ray.direction) > 0) {
			sh.normal = -sh.normal;
			sh.backfacing = true;
		}

		// Lack of information to fill in the other fields...
	}

	return sh;
}

} // namespace jvl::gfx::cpu
