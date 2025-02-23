#pragma once

#include "transform.hpp"

struct Data {
	glm::vec3 origin;
	glm::vec3 lower_left;
	glm::vec3 horizontal;
	glm::vec3 vertical;
};

struct Aperature {
	float aspect = 1.0f;
	float fovy = 45.0f;
	float near = 0.1f;
	float far = 10000.0f;

	Data rayframe(const Transform &) const;

	glm::mat4 perspective() const;
};

inline Data Aperature::rayframe(const Transform &transform) const
{
	auto [forward, right, up] = transform.axes();

	// Convert FOV to radians
	float vfov = glm::radians(fovy);

	float h = std::tan(vfov / 2);

	float vheight = 2 * h;
	float vwidth = vheight * aspect;

	glm::vec3 w = glm::normalize(-forward);
	glm::vec3 u = glm::normalize(glm::cross(up, w));
	glm::vec3 v = glm::cross(w, u);

	glm::vec3 horizontal = u * vwidth;
	glm::vec3 vertical = v * vheight;

	return Data {
		.origin = transform.translate,
		.lower_left = transform.translate - horizontal/2.0f - vertical/2.0f - w,
		.horizontal = horizontal,
		.vertical = vertical
	};
}

inline glm::mat4 Aperature::perspective() const
{
	const float rad = glm::radians(fovy);
	const float tan_half_fovy = std::tan(rad / 2.0f);

	glm::mat4 ret(1.0f);
	ret[0][0] = 1.0f / (aspect * tan_half_fovy);
	ret[1][1] = 1.0f / (tan_half_fovy);
	ret[2][2] = -(far + near) / (far - near);
	ret[2][3] = -1.0f;
	ret[3][2] = -2.0f * (far * near) / (far - near);
	return ret;
}