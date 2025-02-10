#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform {
	glm::vec3 translate = glm::vec3(0, 0, 0);
	glm::vec3 scale = glm::vec3(1, 1, 1);
	glm::quat rotation = glm::quat(0, 0, 0, 1);

	glm::vec3 forward() const {
		return glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
	}

	glm::vec3 right() const {
		return glm::normalize(glm::vec3(rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
	}

	glm::vec3 up() const {
		return glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
	}

	glm::mat4 matrix() const {
		glm::mat4 pmat = glm::translate(glm::mat4(1.0f), translate);
		glm::mat4 rmat = glm::mat4_cast(rotation);
		glm::mat4 smat = glm::scale(glm::mat4(1.0f), scale);
		return pmat * rmat * smat;
	}

	glm::mat4 view_matrix() const {
		return glm::lookAt(translate, translate + forward(), up());
	}

	std::tuple <glm::vec3, glm::vec3, glm::vec3> axes() const {
		return { forward(), right(), up() };
	}
};