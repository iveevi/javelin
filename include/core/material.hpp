#pragma once

#include "../types.hpp"

namespace jvl::core {

template <typename T>
using potentially_texture = jvl::wrapped::variant <std::string, T>;

template <typename T>
concept material = requires(const property <property_value> &values) {
	{
		T::from(values)
	} -> std::same_as <wrapped::optional <T>>;
};

// Generic material description
struct Material {
	static constexpr const char *brdf_key = "brdf";
	static constexpr const char *ambient_key = "ambient";
	static constexpr const char *diffuse_key = "diffuse";
	static constexpr const char *specular_key = "specular";
	static constexpr const char *emission_key = "emission";
	static constexpr const char *roughness_key = "roughness";

	property <property_value> values;

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

	static Phong from(const Material &material) {
		Phong phong = null();

		// TODO: elif if texture variant (check texture first)
		if (auto opt_kd = material.values.get(Material::diffuse_key).as <float3> ())
			phong.kd = opt_kd.value();

		if (auto opt_ks = material.values.get(Material::specular_key).as <float3> ())
			phong.ks = opt_ks.value();

		if (auto opt_roughness = material.values.get(Material::roughness_key).as <float> ())
			phong.roughness = opt_roughness.value();

		if (auto opt_emission = material.values.get(Material::emission_key).as <float3> ())
			phong.emission = opt_emission.value();

		return phong;
	}
};

} // namespace jvl::core
