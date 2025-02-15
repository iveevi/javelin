#include <ire.hpp>

#include "gl.hpp"

MODULE(gl);

bool init_context()
{
	static bool initialized = false;
	if (initialized)
		return true;
		
	JVL_INFO("configuring GL/GLFW context.");

	if (!glfwInit()) {
		JVL_ERROR("failed to initiliaze GLFW");
		return false;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(800, 800, "JVL:TEST", NULL, NULL);
	if (window == NULL) {
		JVL_ERROR("failed to create GLFW window");
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
		JVL_ERROR("failed to load GLAD");
		return false;
	}

	return (initialized = true);
}