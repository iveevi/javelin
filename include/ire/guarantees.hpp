#pragma once

#include "primitive.hpp"
#include "layout_io.hpp"
#include "vector.hpp"

namespace jvl::ire {

static_assert(synthesizable <primitive_t <bool>>);
static_assert(synthesizable <primitive_t <int>>);
static_assert(synthesizable <primitive_t <float>>);

static_assert(synthesizable <vec <bool, 2>>);
static_assert(synthesizable <vec <bool, 3>>);
static_assert(synthesizable <vec <bool, 4>>);

static_assert(synthesizable <vec <int, 2>>);
static_assert(synthesizable <vec <int, 3>>);
static_assert(synthesizable <vec <int, 4>>);

static_assert(synthesizable <vec <float, 2>>);
static_assert(synthesizable <vec <float, 3>>);
static_assert(synthesizable <vec <float, 4>>);

static_assert(synthesizable <layout_in <bool, 0>>);
static_assert(synthesizable <layout_in <int, 0>>);
static_assert(synthesizable <layout_in <float, 0>>);

} // namespace jvl::ire
