#pragma once

#include "uniform_layout.hpp"
#include "tagged.hpp"
#include "primitive.hpp"

namespace jvl::ire {

template <primitive_type T>
constexpr primitive_t <T> upcast(const T &) { return primitive_t <T> (); }

template <uniform_compatible T>
constexpr T upcast(const T &) { return T(); }

template <synthesizable T>
constexpr T upcast(const T &) { return T(); }

template <typename T>
concept global_qualifier_compatible = primitive_type <T> || uniform_compatible <T> || synthesizable <T>;

} // namespace jvl::ire
