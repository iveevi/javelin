#pragma once

#include "../../math_types.hpp"

namespace jvl::gfx::cpu {

struct Intersection {
	float time;
	float3 normal;

	// TODO: && version
	void update(const Intersection &other) {
		if (other.time < 0)
			return;

		if (time < 0 || time > other.time) {
			time = other.time;
			normal = other.normal;
		}
	}

	static Intersection miss() {
		Intersection sh;
		sh.time = -1;
		return sh;
	}
};

inline Intersection ray_triangle_intersection(const float3 &ray,
		                              const float3 &origin,
					      const float3 &v0,
					      const float3 &v1,
					      const float3 &v2)
{
	Intersection sh = Intersection::miss();

	float3 e1 = v1 - v0;
	float3 e2 = v2 - v0;
	float3 p = cross(ray, e2);

	float a = dot(e1, p);
	if (std::abs(a) < 1e-6)
		return sh;

	float f = 1.0 / a;
	float3 s = origin - v0;
	float u = f * dot(s, p);

	if (u < 0.0 || u > 1.0)
		return sh;

	float3 q = cross(s, e1);
	float v = f * dot(ray, q);

	if (v < 0.0 || u + v > 1.0)
		return sh;

	float t = f * dot(e2, q);
	if (t > 0.00001) {
		sh.time = t;
		// sh.p = origin + t * ray;
		// sh.b = float3 {1 - u - v, u, v};

		// if (triangle.has_uvs)
		// 	sh.uv = uv0 * sh.b.x + triangle.uv1 * sh.b.y + triangle.uv2 * sh.b.z;
		// else
		// 	sh.uv = Vector2 { sh.b.y, sh.b.z };

		sh.normal = normalize(cross(e1, e2));
		// if (dot(sh.ng, ray) > 0) {
		// 	sh.ng = -sh.ng;
		// 	sh.backfacing = true;
		// }
		//
		// if (triangle.has_normals) {
		// 	sh.ns = triangle.n0 * sh.b.x + triangle.n1 * sh.b.y + triangle.n2 * sh.b.z;
		// 	if (dot(sh.ns, ray) > 0)
		// 		sh.ns = -sh.ns;
		// } else {
		// 	sh.ns = sh.ng;
		// }
	}

	return sh;
}

} // namespace jvl::gfx::cpu
