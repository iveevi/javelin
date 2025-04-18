#pragma once

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
#include "ire/push_constant.hpp"
#include "ire/qualified_wrapper.hpp"
#include "ire/sampler.hpp"
#include "ire/shared.hpp"
#include "ire/solidify.hpp"
#include "ire/special.hpp"
#include "ire/std.hpp"
#include "ire/transformations.hpp"
#include "ire/vector.hpp"

#include "ire/procedure/ordinary.hpp"
#include "ire/procedure/partial.hpp"

#include "ire/intrinsics/common.hpp"
#include "ire/intrinsics/glsl.hpp"
#include "ire/intrinsics/stages/mesh.hpp"
#include "ire/intrinsics/stages/raytracing.hpp"

// TODO: organize into different targets, i.e. core/glsl.hpp, core/spirv.hpp, etc.
// TODO: debug only
#include "ire/guarantees.hpp"