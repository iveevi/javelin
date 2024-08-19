#include <cassert>
#include <random>

#include "core/aperature.hpp"
#include "core/material.hpp"
#include "core/ray.hpp"
#include "core/transform.hpp"
#include "gfx/cpu/scene.hpp"
#include "math_types.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::gfx;

inline float3 reflect(const float3 &v, const float3 &n)
{
    return normalize(v - 2 * dot(v, n) * n);
}

inline std::pair <float3, float> sample_hemisphere(float u, float v)
{
	assert(u >= 0 && u < 1);
	assert(v >= 0 && v < 1);

	float phi = 2 * M_PI * u;
	float z = sqrt(1 - v);

	return {
		float3 {
			std::cos(phi) * std::sqrt(v),
			std::sin(phi) * std::sqrt(v),
			z
		},
		z/M_PI
	};
}

struct OrthonormalBasis {
	float3 u, v, w;

	float3 local(const float3 &a) const {
		return a.x * u + a.y * v + a.z * w;
	}

	static OrthonormalBasis from(const float3 &n) {
		OrthonormalBasis onb;
		onb.w = normalize(n);

		float3 a = (fabs(onb.w.x) > 0.9) ? float3 { 0, 1, 0 } : float3 { 1, 0, 0 };
		onb.v = normalize(cross(onb.w, a));
		onb.u = cross(onb.w, onb.v);

		return onb;
	}
};

struct HostRaytracer {
	core::Transform transform;
	core::Aperature aperature;
	core::Rayframe rayframe;

	cpu::Scene &scene;

	std::mt19937 random;
	std::uniform_real_distribution <float> distribution;

	HostRaytracer(cpu::Scene &scene_)
			: scene(scene_), random(std::random_device {} ()), distribution(0, 1) {}

	float3 radiance(const Ray &ray, int depth = 0) {
		if (depth <= 0)
			return float3(0.0f);

		// TODO: trace with min/max time for shadows...
		auto sh = scene.trace(ray);

		// TODO: get environment map lighting from scene...
		if (sh.time <= 0.0)
			return float3(0.0f);

		assert(sh.material < scene.materials.size());
		Material &material = scene.materials[sh.material];

		Phong phong = Phong::from(material);
		float3 emission = phong.emission.as <float3> ();
		if (length(emission) > 0)
			return emission;

		float3 kd = phong.kd.as <float3> ();

		constexpr float epsilon = 1e-3f;

		float u = distribution(random);
		float v = distribution(random);

		auto [sampled, pdf] = sample_hemisphere(u, v);

		auto onb = OrthonormalBasis::from(sh.normal);

		Ray next = ray;
		next.origin = sh.point + epsilon * sh.normal;
		next.direction = onb.local(sampled);

		float lambertian = std::max(dot(sh.normal, next.direction), 0.0f);
		float3 brdf = lambertian * (kd/pi <float>);
		float3 Le = radiance(next, depth - 1);
		float3 ret = Le * kd;
		return brdf * Le/pdf;
	}

	float3 radiance_samples(const Ray &ray, size_t samples) {
		float3 summed = 0;
		for (size_t i = 0; i < samples; i++)
			summed += radiance(ray, 6);

		return summed/float(samples);
	}

	Ray ray_from_pixel(float2 uv) {
		core::Ray ray;
		ray.origin = rayframe.origin;
		ray.direction = normalize(rayframe.lower_left
				+ uv.x * rayframe.horizontal
				+ (1 - uv.y) * rayframe.vertical
				- rayframe.origin);

		return ray;
	}
};
