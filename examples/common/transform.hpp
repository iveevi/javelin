#pragma once

#include <core/math.hpp>

struct Transform {
	jvl::float3 translate = jvl::float3(0, 0, 0);
	jvl::float3 scale = jvl::float3(1, 1, 1);
	jvl::fquat rotation = jvl::fquat(0, 0, 0, 1);

	jvl::float3 forward() const {
		constexpr jvl::float3 forwardv { 0, 0, 1 };
		return rotation.rotate(forwardv);
	}

	jvl::float3 right() const {
		constexpr jvl::float3 rightv { 1, 0, 0 };
		return rotation.rotate(rightv);
	}

	jvl::float3 up() const {
		constexpr jvl::float3 upv { 0, 1, 0 };
		return rotation.rotate(upv);
	}

	jvl::float4x4 to_mat4() const {
		jvl::float4x4 tmat = translate_to_mat4(translate);
		jvl::float4x4 rmat = rotation_to_mat4(rotation);
		jvl::float4x4 smat = scale_to_mat4(scale);
		return tmat * rmat * smat;
	}

	jvl::float4x4 to_view_matrix() const {
		constexpr jvl::float3 forward { 0, 0, 1 };
		constexpr jvl::float3 up { 0, 1, 0 };
		return look_at(translate,
			translate + rotation.rotate(forward),
			rotation.rotate(up));
	}

	std::tuple <jvl::float3, jvl::float3, jvl::float3> axes() const {
		// X is horizontal
		// Y is vertical
		// Z is front/back
		constexpr jvl::float3 forward {0, 0, 1};
		constexpr jvl::float3 right {1, 0, 0};
		constexpr jvl::float3 up {0, 1, 0};

		return {
			rotation.rotate(forward),
			rotation.rotate(right),
			rotation.rotate(up),
		};
	}
};

// TODO: caching transform with dirty bit