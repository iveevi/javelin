#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <gtest/gtest.h>

#include "ire/core.hpp"

using namespace jvl;
using namespace jvl::ire;

// TODO: using vulkan as well...
bool init_context()
{
	static bool initialized = false;
	if (initialized)
		return true;

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(800, 800, "JVL:TEST", NULL, NULL);
	if (window == NULL) {
		printf("failed to load glfw\n");
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
		printf("failed to load glad\n");
		return false;
	}

	return (initialized = true);
}

void check_glsl_source(std::string &source, GLuint stage)
{
	ASSERT_TRUE(init_context());

	GLuint program = glCreateShader(stage);
	const char *source_c_str = source.c_str();
	glShaderSource(program, 1, &source_c_str, nullptr);
	glCompileShader(program);

	int success;
	char log[512];

	glGetShaderiv(program, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(program, 512, NULL, log);
		fmt::println("compilation error:\n{}", log);
	}

	ASSERT_TRUE(success);
}

template <typename T>
void simple_io()
{
	auto shader = []() {
		layout_in <T, 0> lin;
		layout_out <T, 0> lout;
		lout = lin;
	};

	auto kernel = kernel_from_args(shader);
	auto glsl = kernel.synthesize(profiles::opengl_450);
	check_glsl_source(glsl, GL_VERTEX_SHADER);
}

TEST(ire_synthesize_glsl, simple_int)
{
	simple_io <int> ();
}

TEST(ire_synthesize_glsl, simple_float)
{
	simple_io <float> ();
}

template <typename T, size_t N>
void simple_vector_io()
{
	auto shader = []() {
		layout_in <vec <T, N>, 0> lin;
		layout_out <vec <T, N>, 0> lout;
		lout = lin;
	};

	auto kernel = kernel_from_args(shader);
	auto glsl = kernel.synthesize(profiles::cplusplus_11);
	check_glsl_source(glsl, GL_VERTEX_SHADER);
}

TEST(ire_synthesize_glsl, simple_int_2)
{
	simple_vector_io <int, 2> ();
}

TEST(ire_synthesize_glsl, simple_float_2)
{
	simple_vector_io <float, 2> ();
}

TEST(ire_synthesize_glsl, simple_int_3)
{
	simple_vector_io <int, 3> ();
}

TEST(ire_synthesize_glsl, simple_float_3)
{
	simple_vector_io <float, 3> ();
}

TEST(ire_synthesize_glsl, simple_int_4)
{
	simple_vector_io <int, 4> ();
}

TEST(ire_synthesize_glsl, simple_float_4)
{
	simple_vector_io <float, 4> ();
}