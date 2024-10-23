#pragma once

#include "../types.hpp"
#include "messaging.hpp"

namespace jvl::core {

// General value information
// TODO: color3 which is float3 alias...
// TODO: texture which is a texture location...
using property_value = jvl::wrapped::variant <
	int, float,
	int2, float2,
	int3, float3,
	int4, float4,
	std::string
>;

template <typename T>
concept material = requires(const property <property_value> &values) {
	{
		T::from(values)
	} -> std::same_as <wrapped::optional <T>>;
};

// Generic material description
struct Material : Unique {
	static constexpr const char *brdf_key = "brdf";
	static constexpr const char *ambient_key = "ambient";
	static constexpr const char *diffuse_key = "diffuse";
	static constexpr const char *specular_key = "specular";
	static constexpr const char *emission_key = "emission";
	static constexpr const char *roughness_key = "roughness";

	std::string name;

	property <property_value> values;

	Material(const std::string &name_) : Unique(new_uuid <Material> ()), name(name_) {}

	// TODO: generic evaluation graphs...

	template <material T>
	std::optional <T> specialize() const {
		return T::from(values);
	}
};

} // namespace jvl::core
