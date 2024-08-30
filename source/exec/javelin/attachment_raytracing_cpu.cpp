#include "attachment_raytracing_cpu.hpp"
#include "source/exec/javelin/global_context.hpp"

using namespace jvl;
using namespace core;
using namespace gfx;

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
	float3 u;
	float3 v;
	float3 w;

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

		float3 a = (fabs(onb.w.x) > 0.9)
			? float3 { 0, 1, 0 }
			: float3 { 1, 0, 0 };
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

	Diffuse &load(const core::Material &material) {
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

inline wrapped::optional <float3> emission(const core::Material &material)
{
	if (auto emission = material.values.get("emission"))
		return emission.as <float3> ();

	return std::nullopt;
}

// Construction
AttachmentRaytracingCPU::AttachmentRaytracingCPU(const std::unique_ptr <GlobalContext> &global_context)
{
	auto &drc = global_context->drc;

	scene = gfx::cpu::Scene::from(global_context->scene);
	random = std::mt19937(std::random_device()());
	distribution = std::uniform_real_distribution <float> (0, 1);

	scene.build_bvh();
	fb = std::make_unique <HostFramebufferCollection> (drc.allocator(), drc.commander());
	
	auto kernel = std::bind(&AttachmentRaytracingCPU::kernel, this, std::placeholders::_1);
	thread_pool = std::make_unique <gfx::cpu::fixed_function_thread_pool <Tile>> (8, kernel);
}

bool AttachmentRaytracingCPU::kernel(Tile &tile)
{
	auto processor = std::bind(&AttachmentRaytracingCPU::raytrace, this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3);

	fb->host.process_tile(processor, tile);

	return (++tile.data.accumulated) < tile.data.expected;
}

void AttachmentRaytracingCPU::render_imgui_synthesized()
{
	ImGui::Begin("CPU Raytracer (Output)");

	ImGui::Text("Progress");
	ImGui::SameLine();

	float progress = float(sample_count)/float(sample_total);
	ImGui::ProgressBar(progress);

	ImGui::Separator();

	ImVec2 size = ImGui::GetContentRegionAvail();
	extent = { int(size.x), int(size.y) };
	aperature.aspect = size.x/size.y;
	if (!fb->empty())
		ImGui::Image(fb->descriptor, size);

	ImGui::End();
}

void AttachmentRaytracingCPU::render_imgui_options()
{
	ImGui::Begin("CPU Raytracer (Options)");

	ImGui::Text("Sample count");
	ImGui::InputInt("samples", &samples);

	ImGui::End();
}

void AttachmentRaytracingCPU::trigger(const core::Transform &transform_)
{
	fb->resize(extent.x, extent.y);
	contributions.resize(extent.x, extent.y);

	sample_count = 0;
	sample_total = samples * extent.x * extent.y;

	int2 size { 32, 32 };
	auto tiles = fb->host.tiles <AttachmentRaytracingCPU::Extra> (size, AttachmentRaytracingCPU::Extra(0, samples));

	transform = transform_;
	rayframe = core::rayframe(aperature, transform);

	thread_pool->reset();
	thread_pool->enque(tiles);
	fb->host.clear();
	contributions.clear();
	thread_pool->begin();
}

Attachment AttachmentRaytracingCPU::attachment()
{
	return Attachment {
		.renderer = [&](const RenderState &rs) { fb->refresh(rs.cmd); },
		.resizer = []() { /* Nothing */ },
		.user = this,
	};
}

// Rendering methods
float4 AttachmentRaytracingCPU::raytrace(int2 ij, float2 uv, const Extra &extra)
{
	core::Ray ray = ray_from_pixel(uv);
	float3 color = radiance(ray, 10);

	float3 &prior = contributions[ij];
	prior = prior * float(extra.accumulated);
	prior += color;
	prior = prior / float(extra.accumulated + 1);

	sample_count++;

	return float4(clamp(prior, 0, 1), 1);
}

// Main rendering method
float3 AttachmentRaytracingCPU::radiance(const Ray &ray, int depth)
{
	if (depth <= 0)
		return float3(0.0f);

	// TODO: trace with min/max time for shadows...
	auto sh = scene.trace(ray);

	// TODO: get environment map lighting from scene...
	if (sh.time <= 0.0) {
		float y = 0.2 * ray.direction.y;
		return float3(exp(y/0.1), exp(y/0.3), exp(y/0.6));
	}

	assert(sh.material < scene.materials.size());
	Material &material = scene.materials[sh.material];

	if (auto em = emission(material))
		return em.value();

	auto diffuse = Diffuse(sh).load(material);
	auto scattering = diffuse.sample(-ray.direction);

	core::Ray next = ray;
	next.origin = sh.offset();
	next.direction = scattering.wo;

	return radiance(next, depth - 1) * scattering.brdf/scattering.pdf;
}

float3 AttachmentRaytracingCPU::radiance_samples(const Ray &ray, size_t samples)
{
	float3 summed = 0;
	for (size_t i = 0; i < samples; i++)
		summed += radiance(ray, 6);

	return summed/float(samples);
}

Ray AttachmentRaytracingCPU::ray_from_pixel(float2 uv)
{
	Ray ray;
	ray.origin = rayframe.origin;
	ray.direction = normalize(rayframe.lower_left
			+ uv.x * rayframe.horizontal
			+ (1 - uv.y) * rayframe.vertical
			- rayframe.origin);

	return ray;
}