#pragma once

#include "types.hpp"

// General value information
struct color3 : glm::vec3 {
	using glm::vec3::vec3;

	color3(const glm::vec3 &other) : glm::vec3(other) {}
};

struct name : std::string {
	using std::string::string;

	name(const std::string &other) : std::string(other) {}
};

struct texture : std::string {
	using std::string::string;

	texture(const std::string &other) : std::string(other) {}
};

using material_property = bestd::variant <
	float,
	color3,
	name,
	texture
>;

template <typename T>
concept material = requires(const property <material_property> &values) {
	{
		T::from(values)
	} -> std::same_as <bestd::optional <T>>;
};

// Generic material description
struct ImportedMaterial {
	static constexpr const char *brdf_key = "BRDF";
	static constexpr const char *ambient_key = "Ambient";
	static constexpr const char *diffuse_key = "Diffuse";
	static constexpr const char *specular_key = "Specular";
	static constexpr const char *emission_key = "Emission";
	static constexpr const char *roughness_key = "Roughness";

	std::string name;

	property <material_property> values;

	ImportedMaterial(const std::string &name_) : name(name_) {}
};