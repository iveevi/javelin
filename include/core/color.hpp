#pragma once

#include "../math_types.hpp"

namespace jvl::core {

// HSV palette generation
float3 hsv_to_rgb(const float3 &hsv)
{
	float h = hsv.x;
	float s = hsv.y;
	float v = hsv.z;

	float c = v * s;
	float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
	float m = v - c;

	float r;
	float g;
	float b;
	if (0 <= h && h < 60) {
		r = c;
		g = x;
		b = 0;
	} else if (60 <= h && h < 120) {
		r = x;
		g = c;
		b = 0;
	} else if (120 <= h && h < 180) {
		r = 0;
		g = c;
		b = x;
	} else if (180 <= h && h < 240) {
		r = 0;
		g = x;
		b = c;
	} else if (240 <= h && h < 300) {
		r = x;
		g = 0;
		b = c;
	} else {
		r = c;
		g = 0;
		b = x;
	}

	return float3((r + m), (g + m), (b + m));
}

} // namespace jvl::core