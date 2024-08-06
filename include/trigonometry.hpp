#pragma once

#include "constants.hpp"

namespace jvl {

template <typename T>
constexpr T to_radians(T degrees)
{
	return pi <T> * degrees / T(180.0f);
}

} // namespace jvl
