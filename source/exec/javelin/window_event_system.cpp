#include "window_event_system.hpp"
	
void WindowEventSystem::button_callback(GLFWwindow *window, int button, int action, int mods)
{
	button_event event {
		.button = button,
		.action = action,
		.mods = mods,
	};

	glfwGetCursorPos(window, &event.x, &event.y);

	bool held = false;
	for (auto &[uuid, region] : regions) {
		if (region.contains(event.x, event.y)) {
			if (action == GLFW_PRESS)
				region.drag = true;
			else if (action == GLFW_RELEASE)
				region.drag = false;

			held = true;
		} else {
			region.drag = false;
		}
	}

	if (held)
		return;
	
	ImGuiIO &io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, action);
}

void WindowEventSystem::cursor_callback(GLFWwindow *window, double x, double y)
{
	cursor_event event {
		.x = x,
		.y = y,
		.dx = x - last_x,
		.dy = y - last_y,
	};

	last_x = x;
	last_y = y;

	for (auto &[uuid, region] : regions) {
		if (region.contains(event.x, event.y)) {
			auto re = event;
			re.drag = region.drag;

			if (region.cursor_callback)
				region.cursor_callback.value()(re);
		}
	}
	
	ImGuiIO &io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);
}

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto user = glfwGetWindowUserPointer(window);
	auto event_system = reinterpret_cast <WindowEventSystem *> (user);
	event_system->button_callback(window, button, action, mods);
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	auto user = glfwGetWindowUserPointer(window);
	auto event_system = reinterpret_cast <WindowEventSystem *> (user);
	event_system->cursor_callback(window, x, y);
}