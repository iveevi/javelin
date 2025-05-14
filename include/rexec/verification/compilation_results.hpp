#pragma once

#include "compilation_status.hpp"

namespace jvl::rexec {

// TODO: add bindable resources...
template <typename VertexElement, typename VertexConstants, typename FragmentConstants>
struct VerifyTraditionalResult {
	PipelineCompilationStatus status;

	using vertex = VertexElement;
	using vconst = VertexConstants;
	using fconst = FragmentConstants;
};

} // namespace jvl::rexec