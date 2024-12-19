#pragma once

#include <core/math.hpp>

namespace jvl::core {

struct Transform {
	float3 translate = float3(0, 0, 0);
	float3 scale = float3(1, 1, 1);
	fquat rotation = fquat(0, 0, 0, 1);

	float3 forward() const {
		constexpr float3 forwardv { 0, 0, 1 };
		return rotation.rotate(forwardv);
	}

	float3 right() const {
		constexpr float3 rightv { 1, 0, 0 };
		return rotation.rotate(rightv);
	}

	float3 up() const {
		constexpr float3 upv { 0, 1, 0 };
		return rotation.rotate(upv);
	}

	float4x4 to_mat4() const {
		float4x4 tmat = translate_to_mat4(translate);
		float4x4 rmat = rotation_to_mat4(rotation);
		float4x4 smat = scale_to_mat4(scale);
		return tmat * rmat * smat;
	}

	float4x4 to_view_matrix() const {
		constexpr float3 forward { 0, 0, 1 };
		constexpr float3 up { 0, 1, 0 };
		return look_at(translate,
			translate + rotation.rotate(forward),
			rotation.rotate(up));
	}

	std::tuple <float3, float3, float3> axes() const {
		// X is horizontal
		// Y is vertical
		// Z is front/back
		constexpr float3 forward {0, 0, 1};
		constexpr float3 right {1, 0, 0};
		constexpr float3 up {0, 1, 0};

		return {
			rotation.rotate(forward),
			rotation.rotate(right),
			rotation.rotate(up),
		};
	}
};

// TODO: caching transform with dirty bit

} // namespace jvl::core
