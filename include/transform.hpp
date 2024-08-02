#pragma once

#include "math_types.hpp"

namespace jvl {

struct Transform {
	float3 translate = float3(0, 0, 0);
	float3 scale     = float3(1, 1, 1);
	fquat rotation   = fquat(1, 0, 0, 0);

	float4x4 to_mat4() const {
		float4x4 rot = rotation.to_mat3();
	}
};

}
