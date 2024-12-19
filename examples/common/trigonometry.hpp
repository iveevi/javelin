#pragma once

#include <concepts>

#include "constants.hpp"

namespace jvl {

template <std::floating_point T>
constexpr T to_radians(T degrees)
{
	return pi <T> * degrees / T(180.0f);
}

} // namespace jvl
