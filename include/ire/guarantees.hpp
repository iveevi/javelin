#pragma once

#include "primitive.hpp"
#include "layout_io.hpp"
#include "vector.hpp"
#include "matrix.hpp"

namespace jvl::ire {

static_assert(builtin <native_t <bool>>);
static_assert(builtin <native_t <int>>);
static_assert(builtin <native_t <float>>);

static_assert(builtin <vec <bool, 2>>);
static_assert(builtin <vec <bool, 3>>);
static_assert(builtin <vec <bool, 4>>);

static_assert(builtin <vec <int, 2>>);
static_assert(builtin <vec <int, 3>>);
static_assert(builtin <vec <int, 4>>);

static_assert(builtin <vec <float, 2>>);
static_assert(builtin <vec <float, 3>>);
static_assert(builtin <vec <float, 4>>);

static_assert(builtin <mat <float, 2, 2>>);
static_assert(builtin <mat <float, 3, 3>>);
static_assert(builtin <mat <float, 4, 4>>);

static_assert(builtin <layout_in <bool>>);
static_assert(builtin <layout_in <int>>);
static_assert(builtin <layout_in <float>>);

} // namespace jvl::ire
