#pragma once

#include "tagged.hpp"
#include "layout.hpp"
#include "native.hpp"

namespace jvl::ire {

// Fundamental types for IRE
template <typename T>
concept generic = builtin <T> || aggregate <T>;

template <typename T>
concept generic_or_void = generic <T> || std::same_as <T, void>;

} // namespace jvl::ire
