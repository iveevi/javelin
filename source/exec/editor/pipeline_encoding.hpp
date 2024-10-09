#pragma once

#include "viewport.hpp"

// Pipeline encoding
struct PipelineEncoding {
	ViewportMode mode;

	// Vertex flags required for the pipeline
	vulkan::VertexFlags vertex_flags;

	// For albedo, this indicates the material flags
	uint64_t specialization;

	PipelineEncoding(const ViewportMode &mode_,
			 const vulkan::VertexFlags &flags,
			 const uint64_t &specialization_ = 0)
		: mode(mode_), vertex_flags(flags), specialization(specialization_) {}

	// Comparison operators
	bool operator==(const PipelineEncoding &other) const {
		return (mode == other.mode)
			&& (vertex_flags == other.vertex_flags)
			&& (specialization == other.specialization);
	}

	bool operator<(const PipelineEncoding &other) const {
		if (mode < other.mode)
			return true;
		else if (mode > other.mode)
			return false;

		if (vertex_flags < other.vertex_flags)
			return true;
		if (vertex_flags > other.vertex_flags)
			return false;

		return (specialization < other.specialization);
	}
};