#pragma once

#include "tagged.hpp"
#include "uniform_layout.hpp"
#include "native.hpp"

namespace jvl::ire {

// Fundamental types for IRE
template <typename T>
concept generic = native <T> || builtin <T> || aggregate <T>;

template <typename T>
concept non_native_generic = builtin <T> || aggregate <T>;

template <typename T>
concept void_or_non_native_generic = builtin <T> || aggregate <T> || std::same_as <T, void>;

} // namespace jvl::ire