#pragma once

#include "uniform_layout.hpp"
#include "push_constant.hpp"
#include "layout_io.hpp"
#include "aliases.hpp"
#include "control_flow.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "emitter.hpp"
#include "callable.hpp"

#include "intrinsics/common.hpp"
#include "intrinsics/glsl.hpp"

// TODO: organize into different targets, i.e. core/glsl.hpp, core/spirv.hpp, etc.
// TODO: debug only
#include "guarantees.hpp"
