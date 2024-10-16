#pragma once

#include "imports.hpp"
#include "intrinsics.hpp"

namespace jvl::tsg {

// Lifting shader arguments
template <typename T>
struct lift_argument {
	using type = T;
};

template <generic T>
struct lift_argument <T> {
	using type = ire::layout_in <T>;
};

template <typename ... Args>
struct lift_argument <std::tuple <Args...>> {
	using type = std::tuple <typename lift_argument <Args> ::type...>;
};

// Lifting shader outputs
template <typename T>
struct lift_result {
	using type = T;
};

template <>
struct lift_result <position> {
	using type = position;
};

template <generic T>
struct lift_result <T> {
	using type = ire::layout_out <T>;
};

template <typename ... Args>
struct lift_result <std::tuple <Args...>> {
	using type = std::tuple <typename lift_result <Args> ::type...>;
};

} // namespace jvl::tsg