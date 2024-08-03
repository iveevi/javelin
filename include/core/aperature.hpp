#pragma once

#include <cmath>

#include "transform.hpp"

namespace jvl::core {

struct Aperature {
	float aspect = 1.0f;
	float fovy = 45.0f;
	float near = 0.1f;
	float far = 10000.0f;
};

// TODO: move this to constants
template <typename T>
constexpr T pi = 3.1415926535897932384626433832795028841971;

template <typename T>
constexpr T to_radians(T degrees)
{
	return pi <T> * degrees / T(180.0f);
}

inline float4x4 perspective(const Aperature &apr)
{
	const float rad = to_radians(apr.fovy);
	const float tan_half_fovy = std::tan(rad/2.0f);

	float4x4 ret;
	ret[0][0] = 1.0f / (apr.aspect * tan_half_fovy);
	ret[1][1] = 1.0f / (tan_half_fovy);
	ret[2][2] = -(apr.far + apr.near) / (apr.far - apr.near);
	ret[2][3] = -1.0f;
	ret[3][2] = -2.0f * (apr.far * apr.near) / (apr.far - apr.near);
	return ret;
}

}
