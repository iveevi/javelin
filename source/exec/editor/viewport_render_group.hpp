#pragma once

#include <ire/core.hpp>

#include "pipeline_encoding.hpp"
#include "shaders.hpp"
#include "viewport.hpp"
#include "adaptive_descriptor.hpp"

using namespace jvl;
using namespace jvl::ire;

class ViewportRenderGroup {
	void configure_render_pass(DeviceResourceCollection &);
	void configure_pipeline_mode(DeviceResourceCollection &, RenderMode);
	void configure_pipeline_albedo(DeviceResourceCollection &, bool);
	void configure_pipeline_backup(DeviceResourceCollection &, vulkan::VertexFlags);
	void configure_pipelines(DeviceResourceCollection &);

	// Preparations for rendering pipelines
	void prepare_albedo(const RenderingInfo &);

	// Rendering specific groups of pipelines
	void render_albedo(const RenderingInfo &, const Viewport &, const solid_t <MVP> &);
	void render_objects(const RenderingInfo &, const Viewport &, const solid_t <MVP> &);
	void render_default(const RenderingInfo &, const Viewport &, const solid_t <MVP> &);
public:
	vk::RenderPass render_pass;

	// Pipeline kinds
	std::map <PipelineEncoding, littlevk::Pipeline> pipelines;

	// Tracking descriptor sets
	// TODO: material to descritor table...
	std::map <uint64_t, AdaptiveDescriptor> descriptors;

	// Constructors
	ViewportRenderGroup(DeviceResourceCollection &);

	// Rendering
	void render(const RenderingInfo &, Viewport &);

	// ImGui rendering
	void popup_debug_pipelines(const RenderingInfo &);
	imgui_callback callback_debug_pipelines();
};