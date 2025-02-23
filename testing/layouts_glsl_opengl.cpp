#include <gtest/gtest.h>

#include <ire.hpp>

#include "gl.hpp"

using namespace jvl;
using namespace jvl::ire;

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

template <native T>
void simple_layouts()
{
	if (!init_context())
		GTEST_SKIP();

	auto shader = []() {
		layout_in <native_t <T>> lin(0);
		layout_out <native_t <T>> lout(0);
		lout = lin;
	};

	auto F = procedure("main") << shader;
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

	auto F = procedure("main") << shader;
	auto glsl = link(F).generate_glsl();

	check_glsl_source(glsl, GL_VERTEX_SHADER);
}

TEST(layouts_glsl_opengl, simple_int)
{
	simple_layouts <int> ();
}

TEST(layouts_glsl_opengl, simple_float)
{
	simple_layouts <float> ();
}

TEST(layouts_glsl_opengl, simple_int_2)
{
	simple_layouts <int, 2> ();
}

TEST(layouts_glsl_opengl, simple_float_2)
{
	simple_layouts <float, 2> ();
}

TEST(layouts_glsl_opengl, simple_int_3)
{
	simple_layouts <int, 3> ();
}

TEST(layouts_glsl_opengl, simple_float_3)
{
	simple_layouts <float, 3> ();
}

TEST(layouts_glsl_opengl, simple_int_4)
{
	simple_layouts <int, 4> ();
}

TEST(layouts_glsl_opengl, simple_float_4)
{
	simple_layouts <float, 4> ();
}