#pragma once

#include <ire/core.hpp>

#include "viewport.hpp"

using namespace jvl;
using namespace jvl::ire;

// Pipeline encoding
struct PipelineEncoding {
	ViewportMode mode;

	// For albedo, this indicates the material flags
	uint64_t specialization;

	PipelineEncoding(const ViewportMode &mode_, uint64_t specialization_ = 0)
		: mode(mode_), specialization(specialization_) {}

	// Comparison operators
	bool operator==(const PipelineEncoding &other) const {
		return (mode == other.mode)
			&& (specialization == other.specialization);
	}

	bool operator<(const PipelineEncoding &other) const {
		if (mode < other.mode)
			return true;

		return (mode == other.mode) && (specialization < other.specialization);
	}
};

class ViewportRenderGroup {
	void configure_render_pass(DeviceResourceCollection &);
	void configure_pipeline_mode(DeviceResourceCollection &, ViewportMode);
	void configure_pipeline_albedo(DeviceResourceCollection &, vulkan::MaterialFlags);
	void configure_pipelines(DeviceResourceCollection &);
public:
	vk::RenderPass render_pass;

	// Pipeline kinds
	std::map <PipelineEncoding, littlevk::Pipeline> pipelines;

	// Tracking descriptor sets
	std::map <uint64_t, vk::DescriptorSet> descriptors;

	// Constructors
	ViewportRenderGroup() = default;
	ViewportRenderGroup(DeviceResourceCollection &);

	void render(const RenderingInfo &, Viewport &);
};