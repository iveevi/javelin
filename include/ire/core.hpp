#pragma once

#include "aliases.hpp"
#include "arithmetic.hpp"
#include "array.hpp"
#include "buffer_objects.hpp"
#include "control_flow.hpp"
#include "emitter.hpp"
#include "jit.hpp"
#include "layout_io.hpp"
#include "linking.hpp"
#include "matrix.hpp"
#include "procedure.hpp"
#include "sampler.hpp"
#include "shared.hpp"
#include "solidify.hpp"
#include "uniform_layout.hpp"
#include "vector.hpp"

#include "intrinsics/common.hpp"
#include "intrinsics/glsl.hpp"

// TODO: organize into different targets, i.e. core/glsl.hpp, core/spirv.hpp, etc.
// TODO: debug only
#include "guarantees.hpp"