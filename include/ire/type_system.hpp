#pragma once

#include "tagged.hpp"
#include "uniform_layout.hpp"
#include "primitive.hpp"

namespace jvl::ire {

// Fundamental types for IRE
template <typename T>
concept generic = native <T> || builtin <T> || aggregate <T>;

template <typename T>
concept non_trivial_generic = builtin <T> || aggregate <T>;

template <typename T>
concept non_trivial_generic_or_void = builtin <T> || aggregate <T> || std::same_as <T, void>;

} // namespace jvl::ire