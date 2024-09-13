#include <imgui/imgui.h>

#include "core/transform.hpp"
#include "constants.hpp"

#include "glio.hpp"

using namespace jvl;

ViewportRegion viewport_region;
MouseInfo mouse;

void button_callback(GLFWwindow *window, int button, int action, int mods)
{
	double xpos;
	double ypos;

	glfwGetCursorPos(window, &xpos, &ypos);
	if (!viewport_region.contains(float2(xpos, ypos))) {
		ImGuiIO &io = ImGui::GetIO();
		io.AddMouseButtonEvent(button, action);
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		mouse.left_drag = (action == GLFW_PRESS);
		if (action == GLFW_RELEASE)
			mouse.voided = true;
	}
}

void cursor_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (!viewport_region.contains(float2(xpos, ypos))) {
		ImGuiIO &io = ImGui::GetIO();
		io.MousePos = ImVec2(xpos, ypos);
		return;
	}

	auto transform = reinterpret_cast <core::Transform *> (glfwGetWindowUserPointer(window));

	if (mouse.voided) {
		mouse.last_x = xpos;
		mouse.last_y = ypos;
		mouse.voided = false;
	}

	float xoffset = xpos - mouse.last_x;
	float yoffset = ypos - mouse.last_y;

	mouse.last_x = xpos;
	mouse.last_y = ypos;

	constexpr float sensitivity = 0.0025f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	static float pitch = 0;
	static float yaw = 0;

	if (mouse.left_drag) {
		pitch -= xoffset;
		yaw += yoffset;

		float pi_e = pi <float> / 2.0f - 1e-3f;
		yaw = std::min(pi_e, std::max(-pi_e, yaw));

		transform->rotation = fquat::euler_angles(yaw, pitch, 0);
	}
}
