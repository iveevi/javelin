#pragma once

namespace jvl::rexec {

enum class PipelineEntrypointVerificationStatus {
	eSuccess,
	eDuplicateEntrypoint,
	eExtraneousEntrypoints,
	eUnexpectedEntrypoint,
	eInsufficientEntrypoints,
};

} // namespace jvl::rexec