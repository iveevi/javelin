#pragma once

#include <glm/glm.hpp>

namespace jvl::core {

struct Ray {
	glm::vec3 direction;
	glm::vec3 origin;
};

} // namespace jvl::core
