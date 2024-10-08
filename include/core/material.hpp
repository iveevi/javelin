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

} // namespace jvl::core
