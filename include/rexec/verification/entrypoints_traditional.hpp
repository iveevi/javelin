#pragma once

#include <cstdlib>

#include "../class.hpp"
#include "../context/fragment.hpp"
#include "../context/vertex.hpp"
#include "entrypoints_status.hpp"

namespace jvl::rexec {

// Returns verification status and indices
// of vertex and fragment shader
struct VerifyEntrypointsTraditionalResult {
	PipelineEntrypointVerificationStatus status;
	uint32_t vidx;
	uint32_t fidx;
};

template <size_t I,
	  bool Vertex,
	  size_t Vidx,
	  bool Fragment,
	  size_t Fidx,
	  ire::entrypoint_class Current,
	  ire::entrypoint_class ... Rest>
constexpr auto verify_entrypoints_traditional() -> VerifyEntrypointsTraditionalResult
{
	using rexec = typename Current::rexec;

	constexpr bool more = sizeof...(Rest);
	
	if constexpr (vertex_class <rexec>) {
		if constexpr (Vertex) {
			return { PipelineEntrypointVerificationStatus::eDuplicateEntrypoint, 0, 0 };
		} else {
			if constexpr (more) {
				return verify_entrypoints_traditional <I + 1, true, I, Fragment, Fidx, Rest...> ();
			} else if constexpr (Fragment) {
				return { PipelineEntrypointVerificationStatus::eSuccess, I, Fidx };
			} else {
				return { PipelineEntrypointVerificationStatus::eInsufficientEntrypoints, 0, 0 };
			}
		}
	} else if constexpr (fragment_class <rexec>) {
		if constexpr (Fragment) {
			return { PipelineEntrypointVerificationStatus::eDuplicateEntrypoint, 0, 0 };
		} else {
			if constexpr (more) {
				return verify_entrypoints_traditional <I + 1, Vertex, Vidx, true, I, Rest...> ();
			} else if constexpr (Vertex) {
				return { PipelineEntrypointVerificationStatus::eSuccess, Vidx, I };
			} else {
				return { PipelineEntrypointVerificationStatus::eInsufficientEntrypoints, 0, 0 };
			}
		}
	} else {
		return { PipelineEntrypointVerificationStatus::eUnexpectedEntrypoint, 0, 0 };
	}
}

} // namespace jvl::rexec