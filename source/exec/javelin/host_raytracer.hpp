#include <cassert>
#include <random>

#include "core/aperature.hpp"
#include "core/material.hpp"
#include "core/ray.hpp"
#include "core/transform.hpp"
#include "gfx/cpu/intersection.hpp"
#include "gfx/cpu/scene.hpp"
#include "math_types.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::gfx;

static float random_uniform_float()
{
	static thread_local std::random_device dev;
	static thread_local std::mt19937 random(dev());
	static thread_local std::uniform_real_distribution <float> distribution;
	return distribution(random);
}

static float2 random_uniform_float2()
{
	return float2 {
		random_uniform_float(),
		random_uniform_float(),
	};
}

static float3 random_uniform_float3()
{
	return float3 {
		random_uniform_float(),
		random_uniform_float(),
		random_uniform_float(),
	};
}

inline float3 reflect(const float3 &v, const float3 &n)
{
    return normalize(v - 2 * dot(v, n) * n);
}

template <typename T>
struct RandomSample {
	T data;
	float pdf;
};

inline RandomSample <float3> sample_hemisphere(float u, float v)
{
	assert(u >= 0 && u < 1);
	assert(v >= 0 && v < 1);

	float phi = 2 * M_PI * u;
	float z = sqrt(1 - v);

	return RandomSample {
		.data = float3 {
			std::cos(phi) * std::sqrt(v),
			std::sin(phi) * std::sqrt(v),
			z
		},
		.pdf = z/pi <float>
	};
}

struct OrthonormalBasis {
	float3 u, v, w;

	float3 local(const float3 &a) const {
		return a.x * u + a.y * v + a.z * w;
	}

	RandomSample <float3> random_hemisphere(float2 rnd) const {
		auto t = sample_hemisphere(rnd.x, rnd.y);
		return RandomSample <float3> {
			.data = local(t.data),
			.pdf = t.pdf
		};
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

struct ScatteringEvent {
	float3 brdf;
	float3 wo;
	float pdf;
};

struct SurfaceScattering {
	const cpu::Intersection &sh;

	OrthonormalBasis onb;

	SurfaceScattering(const cpu::Intersection &sh_)
			: sh(sh_), onb(OrthonormalBasis::from(sh.normal)) {}
};

struct Diffuse : SurfaceScattering {
	using SurfaceScattering::SurfaceScattering;

	float3 kd;

	Diffuse &load(const Material &material) {
		if (auto diffuse = material.values.get("diffuse"))
			kd = diffuse->as <float3> ();

		return *this;
	}

	float3 brdf(const float3 &wi, const float3 &wo) const {
		float lambertian = std::max(dot(wo, sh.normal), 0.0f);
		return lambertian * (kd/pi <float>);
	}

	float pdf(const float3 &wi, const float3 &wo) const {
		return std::max(dot(wo, sh.normal), 0.0f)/pi <float>;
	}

	ScatteringEvent sample(const float3 &wi) const {
		float2 rnd = random_uniform_float2();
		auto sample = onb.random_hemisphere(rnd);

		return ScatteringEvent {
			.brdf = brdf(wi, sample.data),
			.wo = sample.data,
			.pdf = sample.pdf,
		};
	}
};

inline wrapped::optional <float3> emission(const Material &material)
{
	if (auto emission = material.values.get("emission"))
		return emission.as <float3> ();

	return std::nullopt;
}

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

		if (auto em = emission(material))
			return em.value();

		auto diffuse = Diffuse(sh).load(material);
		auto scattering = diffuse.sample(-ray.direction);

		Ray next = ray;
		next.origin = sh.offset();
		next.direction = scattering.wo;

		return radiance(next, depth - 1) * scattering.brdf/scattering.pdf;
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
