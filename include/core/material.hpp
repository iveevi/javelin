#pragma once

#include "../types.hpp"
#include "messaging.hpp"

namespace jvl::core {

// General value information
struct color3 : float3 {
	using float3::vector;

	color3(const float3 &other) : float3(other) {}
};

struct texture : std::string {
	using std::string::string;

	texture(const std::string &other) : std::string(other) {}
};

using material_property = jvl::wrapped::variant <
	float,
	color3,
	texture
>;

template <typename T>
concept material = requires(const property <material_property> &values) {
	{
		T::from(values)
	} -> std::same_as <wrapped::optional <T>>;
};

// Generic material description
struct Material : Unique {
	static constexpr const char *brdf_key = "BRDF";
	static constexpr const char *ambient_key = "Ambient";
	static constexpr const char *diffuse_key = "Diffuse";
	static constexpr const char *specular_key = "Specular";
	static constexpr const char *emission_key = "Emission";
	static constexpr const char *roughness_key = "Roughness";

	std::string name;

	property <material_property> values;

	Material(const std::string &name_) : Unique(new_uuid <Material> ()), name(name_) {}

	// TODO: generic evaluation graphs...

	template <material T>
	std::optional <T> specialize() const {
		return T::from(values);
	}
};

} // namespace jvl::core
