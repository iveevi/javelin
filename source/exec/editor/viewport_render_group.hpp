#pragma once

#include <ire/core.hpp>
#include <core/adaptive_descriptor.hpp>

#include "pipeline_encoding.hpp"
#include "shaders.hpp"
#include "viewport.hpp"

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
	void render_albedo(const RenderingInfo &, const Viewport &, const solid_t <ViewInfo> &);
	void render_objects(const RenderingInfo &, const Viewport &, const solid_t <ViewInfo> &);
	void render_default(const RenderingInfo &, const Viewport &, const solid_t <ViewInfo> &);
public:
	vk::RenderPass render_pass;

	// Pipeline kinds
	std::map <PipelineEncoding, littlevk::Pipeline> pipelines;

	// Constructors
	ViewportRenderGroup(DeviceResourceCollection &);

	// Rendering
	void render(const RenderingInfo &, Viewport &);

	// ImGui rendering
	void popup_debug_pipelines(const RenderingInfo &);
	imgui_callback callback_debug_pipelines();
};