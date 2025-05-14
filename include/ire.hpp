#pragma once

// Core framework
#include "ire/aliases.hpp"
#include "ire/arithmetic.hpp"
#include "ire/array.hpp"
#include "ire/buffer.hpp"
#include "ire/control_flow.hpp"
#include "ire/emitter.hpp"
#include "ire/image.hpp"
#include "ire/layout_io.hpp"
#include "ire/linking.hpp"
#include "ire/matrix.hpp"
#include "ire/memory.hpp"
#include "ire/mirror/soft.hpp"
#include "ire/mirror/solid.hpp"
#include "ire/push_constant.hpp"
#include "ire/qualified_wrapper.hpp"
#include "ire/sampler.hpp"
#include "ire/shared.hpp"
#include "ire/special.hpp"
#include "ire/std.hpp"
#include "ire/transformations.hpp"
#include "ire/vector.hpp"

// Subroutines and entrypoints
#include "ire/procedure/ordinary.hpp"
#include "ire/procedure/partial.hpp"

// Intrinsic instructions
#include "ire/intrinsics/common.hpp"
#include "ire/intrinsics/glsl.hpp"

// Extensions
#include "ire/ext/descriptor_indexing.hpp"
#include "ire/ext/mesh_shading.hpp"
#include "ire/ext/ray_tracing.hpp"

// Utility methods (e.g. for debugging)
#include "common/debug.hpp"
#include "common/io.hpp"

// Additional miscellaneous specializations
namespace jvl::ire {
	
template <builtin T>
struct buffer_index <T> {
	static thunder::Index of(const T &value) {
		return value.synthesize().id;
	}
};

} // namespace jvl::ire