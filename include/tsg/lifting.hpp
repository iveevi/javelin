#pragma once

#include "imports.hpp"
#include "intrinsics.hpp"

namespace jvl::tsg {

//////////////////////////////
// Lifting shader arguments //
//////////////////////////////

template <typename T>
struct lift_argument {
	using type = T;
};

template <generic T>
struct lift_argument <T> {
	using type = ire::layout_in <T>;
};

template <generic T, size_t Offset>
struct lift_argument <PushConstant <T, Offset>> {
	using type = ire::push_constant <T>;
};

template <typename ... Args>
struct lift_argument <std::tuple <Args...>> {
	using type = std::tuple <typename lift_argument <Args> ::type...>;
};

////////////////////////////
// Lifting shader outputs //
////////////////////////////

template <typename T>
struct lift_result {
	using single = T;
	using type = std::tuple <single>;
};

template <>
struct lift_result <Position> {
	using single = Position;
	using type = std::tuple <single>;
};

template <generic T>
struct lift_result <T> {
	using single = ire::layout_out <T>;
	using type = std::tuple <single>;
};

template <typename ... Args>
struct lift_result <std::tuple <Args...>> {
	using type = std::tuple <typename lift_result <Args> ::single...>;
};

} // namespace jvl::tsg