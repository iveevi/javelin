#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <gtest/gtest.h>

#include <ire/core.hpp>

using namespace jvl;
using namespace jvl::ire;

MODULE(test-synthesize-glsl);

// TODO: using vulkan as well...
bool init_context()
{
	static bool initialized = false;
	if (initialized) {
		JVL_INFO("GL/GLFW context already initialized.");
		return true;
	}
		
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
void simple_layouts()
{
	if (!init_context())
		GTEST_SKIP();

	auto shader = []() {
		layout_in <T> lin(0);
		layout_out <T> lout(0);
		lout = lin;
	};

	auto F = callable("main") << shader;
	auto glsl = link(F).generate_glsl();

	check_glsl_source(glsl, GL_VERTEX_SHADER);
}

template <typename T, size_t N>
void simple_layouts()
{
	if (!init_context())
		GTEST_SKIP();

	auto shader = []() {
		layout_in <vec <T, N>> lin(0);
		layout_out <vec <T, N>> lout(0);
		lout = lin;
	};

	auto F = callable("main") << shader;
	auto glsl = link(F).generate_glsl();

	check_glsl_source(glsl, GL_VERTEX_SHADER);
}

TEST(ire_synthesize_glsl, simple_int)
{
	simple_layouts <int> ();
}

TEST(ire_synthesize_glsl, simple_float)
{
	simple_layouts <float> ();
}

TEST(ire_synthesize_glsl, simple_int_2)
{
	simple_layouts <int, 2> ();
}

TEST(ire_synthesize_glsl, simple_float_2)
{
	simple_layouts <float, 2> ();
}

TEST(ire_synthesize_glsl, simple_int_3)
{
	simple_layouts <int, 3> ();
}

TEST(ire_synthesize_glsl, simple_float_3)
{
	simple_layouts <float, 3> ();
}

TEST(ire_synthesize_glsl, simple_int_4)
{
	simple_layouts <int, 4> ();
}

TEST(ire_synthesize_glsl, simple_float_4)
{
	simple_layouts <float, 4> ();
}