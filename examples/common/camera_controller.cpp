#include "camera_controller.hpp"

CameraController::CameraController(Transform &transform_,
				   const CameraControllerSettings &binding_)
		: transform(transform_), settings(binding_) {}

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

	// Dragging state
	pitch -= dx * settings.sensitivity / 1e+3f;
	yaw -= dy * settings.sensitivity / 1e+3f;

	float pi_e = glm::pi <float> () / 2.0f - 1e-3f;
	yaw = std::min(pi_e, std::max(-pi_e, yaw));

	transform.rotation = glm::quat(glm::vec3(yaw, pitch, 0));

	return std::abs(dx) > 0 or std::abs(dy) > 0;
}

bool CameraController::handle_delta(glm::vec2 delta)
{
	float dx = delta.x;
	float dy = delta.y;
	
	if (settings.invert_y)
		dy = -dy;

	// Dragging state
	pitch -= dx * settings.sensitivity / 1e+3f;
	yaw -= dy * settings.sensitivity / 1e+3f;

	float pi_e = glm::pi <float> () / 2.0f - 1e-3f;
	yaw = std::min(pi_e, std::max(-pi_e, yaw));

	transform.rotation = glm::quat(glm::vec3(yaw, pitch, 0));

	return std::abs(dx) > 0 or std::abs(dy) > 0;
}

void CameraController::handle_movement(const InteractiveWindow &window)
{
	float delta = settings.speed * float(glfwGetTime() - last_t);
	last_t = glfwGetTime();

	glm::vec3 velocity(0.0f);
	if (window.key_pressed(settings.backward))
		velocity.z -= delta;
	else if (window.key_pressed(settings.forward))
		velocity.z += delta;

	if (window.key_pressed(settings.right))
		velocity.x -= delta;
	else if (window.key_pressed(settings.left))
		velocity.x += delta;

	if (window.key_pressed(settings.down))
		velocity.y -= delta;
	else if (window.key_pressed(settings.up))
		velocity.y += delta;
	
	transform.translate += transform.rotation * velocity;
}