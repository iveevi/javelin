#include "window_event_system.hpp"

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