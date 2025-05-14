#pragma once

#include <cstdlib>

namespace jvl::rexec {

/////////////////////////////////////
// Host-device resource indicators //
/////////////////////////////////////

enum ResourceKind {
	ePushConstant,
	eVertexBuffer,
	eIndexBuffer,
	eStorageBuffer,
	eStorageImage,
	eSampledImage,
};

template <ResourceKind K, typename T, size_t N>
struct ResourceIndicator {
	static constexpr ResourceKind kind = K;
	static constexpr size_t numerical = N;
	using value_type = T;
};

template <typename T, size_t Offset>
using PushConstantIndicator = ResourceIndicator <ePushConstant, T, Offset>;

} // namespace jvl::rexec