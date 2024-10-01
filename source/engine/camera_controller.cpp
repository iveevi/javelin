#include "constants.hpp"
#include "engine/camera_controller.hpp"

namespace jvl::engine {

CameraController::CameraController(core::Transform &transform_,
				   const CameraControllerBinding &binding_)
		: transform(transform_), binding(binding_) {}

void CameraController::handle_cursor(float2 mouse)
{
	if (!dragging)
		return;

	if (voided) {
		last_x = mouse.x;
		last_y = mouse.y;
		voided = false;
	}

	double dx = mouse.x - last_x;
	double dy = mouse.y - last_y;

	// Dragging state
	pitch -= dx * sensitivity;
	yaw -= dy * sensitivity;

	float pi_e = pi <float> / 2.0f - 1e-3f;
	yaw = std::min(pi_e, std::max(-pi_e, yaw));

	last_x = mouse.x;
	last_y = mouse.y;

	transform.rotation = fquat::euler_angles(yaw, pitch, 0);
}

void CameraController::handle_movement(const engine::InteractiveWindow &window)
{
	float delta = speed * float(glfwGetTime() - last_t);
	last_t = glfwGetTime();

	jvl::float3 velocity(0.0f);
	if (window.key_pressed(binding.backward))
		velocity.z -= delta;
	else if (window.key_pressed(binding.forward))
		velocity.z += delta;

	if (window.key_pressed(binding.right))
		velocity.x -= delta;
	else if (window.key_pressed(binding.left))
		velocity.x += delta;

	if (window.key_pressed(binding.down))
		velocity.y -= delta;
	else if (window.key_pressed(binding.up))
		velocity.y += delta;

	transform.translate += transform.rotation.rotate(velocity);
}

} // namespace jvl::engine