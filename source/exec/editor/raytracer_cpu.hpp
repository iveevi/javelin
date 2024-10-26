#pragma once

#include <random>

#include <imgui/imgui.h>

#include <core/aperature.hpp>
#include <gfx/cpu/framebuffer.hpp>
#include <gfx/cpu/scene.hpp>
#include <gfx/cpu/thread_pool.hpp>
#include <logging.hpp>
#include <math_types.hpp>

#include <littlevk/littlevk.hpp>

#include "imgui_render_group.hpp"
#include "viewport.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::gfx;

// Host raytracing context for a fixed resolution, aperature, transform, etc.
struct RaytracerCPU : Unique {
	// Types
	struct Extra {
		int accumulated;
		int expected;
	};

	using Tile = cpu::Tile <Extra>;
	using ThreadPool = cpu::fixed_function_thread_pool <Tile>;

	// Vulkan resources
	littlevk::Buffer staging;
	littlevk::Image display;
	vk::DescriptorSet descriptor;
	vk::Sampler sampler;
	vk::Extent2D extent;

	// Host resources
	cpu::Framebuffer <float4> host;
	cpu::Framebuffer <float3> contributions;

	// Render configuration data
	core::Rayframe rayframe;
	int32_t samples = 1;

	// Rendering structures
	ThreadPool thread_pool;

	cpu::Scene scene;

	// Tracking progress
	std::atomic <int32_t> current_samples = 0;
	int32_t max_samples = 0;

	// Constructors
	RaytracerCPU(DeviceResourceCollection &,
		     const Viewport &,
		     const core::Scene &,
		     const vk::Extent2D &,
		     int32_t  = 1);

	~RaytracerCPU();

	// Rendering methods
	float3 radiance(const Ray &, int);
	float4 raytrace(int2, float2, const Extra &);

	// Rendering kernel
	bool kernel(Tile &);
	std::function <bool (Tile &)> kernel_callback();

	// Refresh the display
	void copy(const vk::CommandBuffer &);
	void refresh(const DeviceResourceCollection &, const vk::CommandBuffer &);

	// Displaying
	void display_handle(const RenderingInfo &);
	imgui_callback callback_display();
};