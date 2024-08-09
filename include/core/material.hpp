#pragma once

#include "../types.hpp"

namespace jvl::core {

// Set of values that can parameterize a material
using material_property_value = jvl::wrapped::variant <
	int, float,
	int2, float2,
	int3, float3,
	int4, float4,
	std::string
>;

template <typename T>
using potentially_texture = jvl::wrapped::variant <std::string, T>;

template <typename T>
concept material = requires(const property <material_property_value> &values) {
	{
		T::from(values)
	} -> std::same_as <wrapped::optional <T>>;
};

// Generic material description
struct Material {
	property <material_property_value> values;

	template <material T>
	std::optional <T> specialize() const {
		return T::from(values);
	}
};

// Specialized materials
// TODO: each material gets compiled to ir before synthesis
struct Phong {
	static constexpr char id[] = "phong";

	potentially_texture <float3> kd;
	potentially_texture <float3> ks;
	potentially_texture <float3> emission;
	potentially_texture <float> roughness;

	static Phong null() {
		Phong phong;
		phong.kd = float3(0, 0, 0);
		phong.ks = float3(0, 0, 0);
		phong.emission = float3(1, 0.5, 1);
		phong.roughness = 0.0f;
		return phong;
	}
};

} // namespace jvl::core
