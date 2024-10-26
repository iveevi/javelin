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

// Convenient type promotion
template <generic T>
struct promote_base {
	using type = T;
};

template <native T>
struct promote_base <T> {
	using type = native_t <T>;
};

template <generic T>
using promoted = typename promote_base <T> ::type;

} // namespace jvl::ire