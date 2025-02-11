#pragma once

#include <string>

#include <glm/glm.hpp>

#include <bestd/tree.hpp>
#include <bestd/variant.hpp>

// General key-value structure
template <typename T>
using property = bestd::tree <std::string, T>;

// General buffer structures
using typed_vector = bestd::variant <
	std::vector <glm::vec4>,
	std::vector <glm::vec3>,
	std::vector <glm::vec2>,
	std::vector <float>,
	std::vector <glm::ivec4>,
	std::vector <glm::ivec3>,
	std::vector <glm::ivec2>,
	std::vector <int>
>;