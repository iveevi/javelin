#include "core/interactive_window.hpp"

namespace jvl::core {

// Interactive window implementations	
InteractiveWindow::InteractiveWindow(const littlevk::Window &window)
		: littlevk::Window(window) {}

bool InteractiveWindow::should_close() const
{
	return glfwWindowShouldClose(handle);
}

bool InteractiveWindow::key_pressed(int key) const
{
	return glfwGetKey(handle, key) == GLFW_PRESS;
}

void InteractiveWindow::poll() const
{
	return glfwPollEvents();
}

} // namespace jvl::core