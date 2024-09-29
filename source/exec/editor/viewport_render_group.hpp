#pragma once

#include <ire/core.hpp>

#include "viewport.hpp"

using namespace jvl;
using namespace jvl::ire;

class ViewportRenderGroup {
	void configure_render_pass(DeviceResourceCollection &);
	void configure_pipeline_mode(DeviceResourceCollection &, ViewportMode);
	void configure_pipeline_albedo(DeviceResourceCollection &, vulkan::MaterialFlags);
	void configure_pipelines(DeviceResourceCollection &);
public:
	vk::RenderPass render_pass;

	// Pipeline kinds
	std::array <littlevk::Pipeline, eCount> pipelines;

	// Constructors
	ViewportRenderGroup() = default;
	ViewportRenderGroup(DeviceResourceCollection &);

	void render(const RenderingInfo &, Viewport &);
};