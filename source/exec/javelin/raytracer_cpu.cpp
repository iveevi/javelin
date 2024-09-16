#include "raytracer_cpu.hpp"
#include "core/ray.hpp"
#include "imgui.h"
#include "source/exec/javelin/imgui_render_group.hpp"

// Raytracing functions
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
	MODULE(sample-hemisphere);

	JVL_ASSERT_PLAIN(u >= 0 && u < 1);
	JVL_ASSERT_PLAIN(v >= 0 && v < 1);

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

	SurfaceScattering(const cpu::Intersection &sh_) : sh(sh_), onb(OrthonormalBasis::from(sh.normal)) {}
};

struct Diffuse : SurfaceScattering {
	using SurfaceScattering::SurfaceScattering;

	float3 kd;

	Diffuse &load(const Material &material) {
		if (auto diffuse = material.values.get("diffuse")) {
			if (diffuse->is <float3> ())
				kd = diffuse->as <float3> ();
			else
				kd = float3(0.6, 0.6, 0.6);
		}

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

// Constructors
RaytracerCPU::RaytracerCPU(DeviceResourceCollection &drc,
			   const Viewport &viewport,
			   const core::Scene &scene_,
			   const vk::Extent2D &extent_,
			   int32_t samples_)
		: uuid(new_uuid <RaytracerCPU> ()),
		extent(extent_),
		samples(samples_),
		thread_pool(8, kernel_callback())
{
	// Initialization
	scene = cpu::Scene::from(scene_);

	// Allocate images and buffers
	host.resize(extent.width, extent.height);
	contributions.resize(extent.width, extent.height);
	
	std::tie(staging, display) = drc.allocator()
		.buffer(host.data, vk::BufferUsageFlagBits::eTransferSrc)
		.image(vk::Extent2D {
				uint32_t(host.width),
				uint32_t(host.height)
			},
			vk::Format::eR32G32B32A32Sfloat,
			vk::ImageUsageFlagBits::eSampled
				| vk::ImageUsageFlagBits::eTransferDst,
			vk::ImageAspectFlagBits::eColor);

	drc.commander().submit_and_wait([&](const vk::CommandBuffer &cmd) {
		copy(cmd);
	});

	sampler = littlevk::SamplerAssembler(drc.device, drc.dal);

	descriptor = imgui::add_vk_texture(sampler, display.view, vk::ImageLayout::eShaderReadOnlyOptimal);

	// Preprocessing
	scene.build_bvh();

	Aperature aperature = viewport.aperature;
	aperature.aspect = float(extent.width)/float(extent.height);
	rayframe = core::rayframe(aperature, viewport.transform);

	current_samples = 0;
	max_samples = samples * extent.width * extent.height;

	auto tiles = host.tiles <Extra> (int2(32, 32), Extra(0, samples));

	thread_pool.reset();
	thread_pool.enque(tiles);
	thread_pool.begin();
}

RaytracerCPU::~RaytracerCPU()
{
	thread_pool.drop();
}

// Rendering methods
Ray RaytracerCPU::ray_from_pixel(float2 uv)
{
	// TODO: rayframe method
	Ray ray;
	ray.origin = rayframe.origin;
	ray.direction = normalize(rayframe.lower_left
			+ uv.x * rayframe.horizontal
			+ (1 - uv.y) * rayframe.vertical
			- rayframe.origin);

	return ray;
}

float3 RaytracerCPU::radiance(const Ray &ray, int depth)
{
	if (depth <= 0)
		return float3(0.0f);

	// TODO: trace with min/max time for shadows...
	auto sh = scene.trace(ray);

	// TODO: get environment map lighting from scene...
	if (sh.time <= 0.0)
		return float3(1, 1, 1);

	assert(sh.material < scene.materials.size());
	Material &material = scene.materials[sh.material];

	if (auto em = emission(material))
		return em.value();

	auto diffuse = Diffuse(sh).load(material);
	auto scattering = diffuse.sample(-ray.direction);

	core::Ray next = ray;
	next.origin = sh.offset();
	next.direction = scattering.wo;

	return radiance(next, depth - 1) * scattering.brdf / scattering.pdf;
}

float4 RaytracerCPU::raytrace(int2 ij, float2 uv, const Extra &extra)
{
	core::Ray ray = ray_from_pixel(uv);
	float3 color = radiance(ray, 10);

	float3 &prior = contributions[ij];
	prior = prior * float(extra.accumulated);
	prior += color;
	prior = prior / float(extra.accumulated + 1);

	current_samples++;

	return float4(clamp(prior, 0, 1), 1);
}

// Rendering kernel
bool RaytracerCPU::kernel(Tile &tile)
{
	auto processor = std::bind(&RaytracerCPU::raytrace, this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3);

	host.process_tile(processor, tile);

	return (++tile.data.accumulated) < tile.data.expected;
}

std::function <bool (RaytracerCPU::Tile &)> RaytracerCPU::kernel_callback()
{
	return std::bind(&RaytracerCPU::kernel, this, std::placeholders::_1);
}

// Refresh the display
void RaytracerCPU::copy(const vk::CommandBuffer &cmd)
{
	display.transition(cmd, vk::ImageLayout::eTransferDstOptimal);

	littlevk::copy_buffer_to_image(cmd,
		display, staging,
		vk::ImageLayout::eTransferDstOptimal);

	display.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void RaytracerCPU::refresh(const DeviceResourceCollection &drc, const vk::CommandBuffer &cmd)
{
	littlevk::upload(drc.device, staging, host.data);
	copy(cmd);
}

// Displaying
void RaytracerCPU::display_handle(const RenderingInfo &info)
{
	std::string title = fmt::format("Raytracer (CPU: {})##{}", uuid.type_local, uuid.global);

	bool open = true;
	
	ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_MenuBar);
	if (!open) {
		Message message {
			.type_id = uuid.type_id,
			.global = uuid.global,
			.kind = editor_remove_self,
		};

		info.message_system.send_to_origin(message);
	}

	if (ImGui::BeginMenuBar()) {
		float progress = float(current_samples)/float(max_samples);
		ImGui::ProgressBar(progress);
		ImGui::EndMenuBar();
	}

	ImGui::Image(descriptor, ImVec2(extent.width, extent.height));
	ImGui::End();
}

imgui_callback RaytracerCPU::imgui_callback()
{
	return {
		uuid.global,
		std::bind(&RaytracerCPU::display_handle, this, std::placeholders::_1)
	};
}