#pragma once

#include <littlevk/littlevk.hpp>

#include "../core/transform.hpp"
#include "../core/interactive_window.hpp"

namespace jvl::engine {

struct CameraControllerSettings {
	int forward = GLFW_KEY_W;
	int backward = GLFW_KEY_S;
	int left = GLFW_KEY_A;
	int right = GLFW_KEY_D;
	int up = GLFW_KEY_Q;
	int down = GLFW_KEY_E;

	bool invert_y = false;
	
	float speed = 100.0f;
	float sensitivity = 2.5f;
};

struct CameraController {
	bool voided = true;
	bool dragging = false;

	float last_t = 0.0f;
	float last_x = 0.0f;
	float last_y = 0.0f;

	float pitch = 0.0f;
	float yaw = 0.0f;

	core::Transform &transform;

	CameraControllerSettings settings;

	CameraController(core::Transform &, const CameraControllerSettings &);

	bool handle_cursor(float2);
	bool handle_delta(float2);
	void handle_movement(const core::InteractiveWindow &);
};

} // namespace jvl::engine