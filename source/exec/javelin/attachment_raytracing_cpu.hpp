#pragma once

#include <atomic>
#include <random>

#include "core/transform.hpp"
#include "core/aperature.hpp"
#include "core/material.hpp"
#include "gfx/cpu/scene.hpp"
#include "gfx/cpu/thread_pool.hpp"
#include "math_types.hpp"

#include "global_context.hpp"
#include "host_framebuffer_collection.hpp"

// Raytracing context for the CPU
struct AttachmentRaytracingCPU {
	static constexpr const char *key = "AttachmentRaytracingCPU";

	jvl::gfx::cpu::Scene scene;

	jvl::core::Transform transform;
	jvl::core::Aperature aperature;
	jvl::core::Rayframe rayframe;

	std::mt19937 random;
	std::uniform_real_distribution <float> distribution;

	jvl::int2 extent;
	int samples = 1;

	// Extension data with framebuffer tiles
	struct Extra {
		int accumulated;
		int expected;
	};

	using Tile = jvl::gfx::cpu::Tile <Extra>;

	// Stored contributions in the making
	jvl::gfx::cpu::Framebuffer <jvl::float3> contributions;

	// Parallelized rendering utilities
	std::unique_ptr <jvl::gfx::cpu::fixed_function_thread_pool <Tile>> thread_pool;
	std::unique_ptr <HostFramebufferCollection> fb;

	// ImGui callbacks
	std::atomic <int> sample_count = 0;
	std::atomic <int> sample_total = 0;

	// Construction
	AttachmentRaytracingCPU(const std::unique_ptr <GlobalContext> &);

	// User interface drawing methods
	void render_imgui_synthesized();
	void render_imgui_options();

	// Trigger the rendering
	void trigger(const jvl::core::Transform &);

	// Global context attachment description
	Attachment attachment();

	// Thread pool worker kernel
	bool kernel(Tile &);

	// Rendering methods	
	jvl::float4 raytrace(jvl::int2, jvl::float2, const Extra &);
	jvl::float3 radiance(const jvl::core::Ray &, int = 0);
	jvl::float3 radiance_samples(const jvl::core::Ray &, size_t);
	jvl::core::Ray ray_from_pixel(jvl::float2);
};