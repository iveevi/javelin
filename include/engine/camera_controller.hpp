#pragma once

#include <littlevk/littlevk.hpp>

#include "../core/transform.hpp"
#include "interactive_window.hpp"

namespace jvl::engine {

struct CameraControllerBinding {
	int forward = GLFW_KEY_W;
	int backward = GLFW_KEY_S;
	int left = GLFW_KEY_A;
	int right = GLFW_KEY_D;
	int up = GLFW_KEY_Q;
	int down = GLFW_KEY_E;
};

struct CameraController {
	bool voided = true;
	bool dragging = false;

	float speed = 100.0f;
	float sensitivity = 0.0025f;

	float last_t = 0.0f;
	float last_x = 0.0f;
	float last_y = 0.0f;

	float pitch = 0.0f;
	float yaw = 0.0f;

	core::Transform &transform;

	CameraControllerBinding binding;

	CameraController(core::Transform &, const CameraControllerBinding &);

	void handle_cursor(float2);
	void handle_movement(const engine::InteractiveWindow &);
};

} // namespace jvl::engine