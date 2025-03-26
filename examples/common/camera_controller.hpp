#pragma once

#include <littlevk/littlevk.hpp>

#include "transform.hpp"
#include "interactive_window.hpp"

struct CameraControllerSettings {
	int forward = GLFW_KEY_W;
	int backward = GLFW_KEY_S;
	int left = GLFW_KEY_A;
	int right = GLFW_KEY_D;
	int up = GLFW_KEY_Q;
	int down = GLFW_KEY_E;

	bool invert_y = false;

	float speed = 500.0f;
	float sensitivity = 2.5f;
};

struct CameraController {
	bool voided = true;
	bool dragging = false;

	float last_t = 0.0f;
	float last_x = 0.0f;
	float last_y = 0.0f;

	float pitch = glm::pi <float> ();
	float yaw = 0;

	Transform &transform;

	CameraControllerSettings settings;

	CameraController(Transform &, const CameraControllerSettings &);

	bool handle_cursor(glm::vec2);
	bool handle_delta(glm::vec2);
	bool handle_movement(const InteractiveWindow &);
};
