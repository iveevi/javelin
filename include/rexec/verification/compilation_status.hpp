#pragma once

#include <cstdint>

#include "../../common/flags.hpp"

namespace jvl::rexec {

enum class PipelineCompilationStatus : uint8_t {
	eSuccess			= 0b0,
	eIncompatibleLayout		= 0b10,
	eMissingLayout			= 0b100,
	eOverlappingPushConstants	= 0b1000,
};

DEFINE_FLAG_OPERATORS(PipelineCompilationStatus, uint8_t)

} // namespace jvl::rexec