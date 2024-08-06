#pragma once

#include "gtest/gtest.h"
#include <cmath>

#include "transform.hpp"
#include "../trigonometry.hpp"

namespace jvl::core {

// Camera aperature types
struct Aperature {
	float aspect = 1.0f;
	float fovy = 45.0f;
	float near = 0.1f;
	float far = 10000.0f;
};

inline float4x4 perspective(const Aperature &apr)
{
	const float rad = to_radians(apr.fovy);
	const float tan_half_fovy = std::tan(rad / 2.0f);

	float4x4 ret;
	ret[0][0] = 1.0f / (apr.aspect * tan_half_fovy);
	ret[1][1] = 1.0f / (tan_half_fovy);
	ret[2][2] = -(apr.far + apr.near) / (apr.far - apr.near);
	ret[2][3] = -1.0f;
	ret[3][2] = -2.0f * (apr.far * apr.near) / (apr.far - apr.near);
	return ret;
}

// Conversion from aperature to a ray generator
struct Rayframe {
	float3 origin;
	float3 lower_left;
	float3 horizontal;
	float3 vertical;
};

inline Rayframe rayframe(const Aperature &apr, const Transform &transform)
{
	auto [forward, right, up] = transform.axes();

	// Convert FOV to radians
        float vfov = to_radians(apr.fovy);

        float h = std::tan(vfov / 2);

        float vheight = 2 * h;
        float vwidth = vheight * apr.aspect;

        float3 w = normalize(-forward);
        float3 u = normalize(cross(up, w));
        float3 v = cross(w, u);

	float3 horizontal = u * vwidth;
        float3 vertical = v * vheight;

	return Rayframe {
		.origin = transform.translate,
		.lower_left = transform.translate - horizontal/2.0f - vertical/2.0f - w,
		.horizontal = horizontal,
		.vertical = vertical
	};
}

} // namespace jvl::core
