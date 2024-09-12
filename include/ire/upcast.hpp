#pragma once

#include "uniform_layout.hpp"
#include "tagged.hpp"
#include "primitive.hpp"

namespace jvl::ire {

template <native T>
constexpr primitive_t <T> upcast(const T &) { return primitive_t <T> (); }

template <aggregate T>
constexpr T upcast(const T &) { return T(); }

template <builtin T>
constexpr T upcast(const T &) { return T(); }

} // namespace jvl::ire
