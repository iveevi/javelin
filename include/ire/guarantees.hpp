#pragma once

#include "gltype.hpp"
#include "layout_io.hpp"

// TODO: import the aliases
namespace jvl::ire {

// static_assert(synthesizable <vec4>);

static_assert(synthesizable <gltype <int>>);

// static_assert(synthesizable <boolean>);

static_assert(synthesizable <layout_in <int, 0>>);

// static_assert(synthesizable <ivec2>);
// static_assert(synthesizable <ivec3>);
// static_assert(synthesizable <ivec4>);
//
// static_assert(synthesizable <vec2>);
// static_assert(synthesizable <vec3>);
// static_assert(synthesizable <vec4>);
//
// static_assert(synthesizable <mat4>);

static_assert(sizeof(op::General) == 24);

} // namespace jvl::ire
