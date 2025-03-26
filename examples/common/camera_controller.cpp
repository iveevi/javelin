#include <fmt/printf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "camera_controller.hpp"

CameraController::CameraController(Transform &transform_,
				   const CameraControllerSettings &binding_)
		: transform(transform_), settings(binding_)
{
	transform.rotation = glm::quat(glm::vec3(yaw, pitch, 0));
}

bool CameraController::handle_cursor(glm::vec2 mouse)
{
	if (!dragging)
		return false;

	if (voided) {
		last_x = mouse.x;
		last_y = mouse.y;
		voided = false;
	}

	double dx = mouse.x - last_x;
	double dy = mouse.y - last_y;

	last_x = mouse.x;
	last_y = mouse.y;

	return handle_delta(glm::vec2(dx, dy));
}

bool CameraController::handle_delta(glm::vec2 delta)
{
	float dx = delta.x;
	float dy = delta.y;

	if (settings.invert_y)
		dy = -dy;

	// Dragging state
	pitch -= dx * settings.sensitivity / 1e3f;
	yaw -= dy * settings.sensitivity / 1e3f;

	float pi_e = glm::pi <float> () / 2.0f - 1e-3f;
	yaw = std::clamp(yaw, -glm::pi <float> () + pi_e, glm::pi <float> () + pi_e);

	transform.rotation = glm::quat(glm::vec3(yaw, pitch, 0));

	return std::abs(dx) > 0 or std::abs(dy) > 0;
}

bool CameraController::handle_movement(const InteractiveWindow &window)
{
	uint32_t unchanged = 0;

	float delta = settings.speed * float(glfwGetTime() - last_t);
	last_t = glfwGetTime();

	glm::vec3 velocity(0.0f);
	if (window.key_pressed(settings.backward))
		velocity.z -= delta;
	else if (window.key_pressed(settings.forward))
		velocity.z += delta;
	else
		unchanged++;

	if (window.key_pressed(settings.right))
		velocity.x -= delta;
	else if (window.key_pressed(settings.left))
		velocity.x += delta;
	else
		unchanged++;

	if (window.key_pressed(settings.down))
		velocity.y -= delta;
	else if (window.key_pressed(settings.up))
		velocity.y += delta;
	else
		unchanged++;

	transform.translate += transform.rotation * velocity;

	return (unchanged < 3);
}
