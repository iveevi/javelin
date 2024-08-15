#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "atom/atom.hpp"
#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/intrinsics/glsl.hpp"
#include "ire/tagged.hpp"
#include "ire/type_synthesis.hpp"
#include "ire/uniform_layout.hpp"
#include "ire/util.hpp"
#include "ire/vector.hpp"
#include "profiles/targets.hpp"

// TODO: synthesizable with name hints
// TODO: std.hpp for additional features

using namespace jvl;
using namespace jvl::ire;

struct lighting {
	vec3 direction;
	vec3 color;
	boolean on;

	auto layout() {
		// TODO: prevent duplicate fields
		// return uniform_layout(direction, direction);
		return uniform_layout(direction, color, on);
	}
};

inline void discard()
{
	platform_intrinsic_keyword("discard");
}

// Smith shadow-masking function
void __flip(vec3 n)
{
	returns(-n);
}

auto flip = callable <vec3> (__flip).named("flip");

void __flip_and_translate(vec3 n, vec3 d)
{
	returns(flip(n) + d);
}

auto flip_and_translate = callable <vec3> (__flip_and_translate).named("flip_and_translate");

void vertex_shader()
{
	// TODO: autodiff on inputs...

	layout_in <vec3, 0> position;
	layout_in <vec3, 1> normal;

	layout_out <vec3, 0> result;

	result = flip_and_translate(position, normal);

	// TODO: immutability for shader inputs
	// TODO: before synthesis, demote variables to inline if they are not modified later
	// TODO: warnings for the unused sections
}

void fragment_shader()
{
	layout_in <vec3, 0> position;
	layout_in <float, 1> depth;

	layout_out <vec4, 0> normal;
	layout_out <vec4, 1> color;

	push_constant <float> tint;

	// TODO: some intrinsics only work on layout ins...
	vec3 dU = dFdx(position);
	vec3 dV = dFdy(position);
	vec3 N = normalize(cross(dU, dV));
	normal = vec4(0.5f + 0.5f * N, 1);
	color = vec4(vec3(depth, depth, depth), tint);
}

// TODO: test on shader toy shaders

#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	auto vertex_kernel = kernel_from_args(vertex_shader);
	std::string vertex_source = vertex_kernel.synthesize(jvl::profiles::opengl_450);
	fmt::println("vsource:\n{}", vertex_source);

	vertex_kernel.dump();

	flip.export_to_kernel().dump();
	flip_and_translate.export_to_kernel().dump();

	// auto fragment_kernel = kernel_from_args(fragment_shader);
	// std::string fragment_source = fragment_kernel.synthesize(jvl::profiles::cplusplus_11);
	// fmt::println("fsource:\n{}", fragment_source);

	// glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	// GLFWwindow *window = glfwCreateWindow(800, 800, "Window", NULL, NULL);
	// if (window == NULL) {
	// 	printf("failed to load glfw\n");
	// 	glfwTerminate();
	// 	return -1;
	// }
	// glfwMakeContextCurrent(window);
	//
	// if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
	// 	printf("failed to load glad\n");
	// 	return -1;
	// }
	//
	// GLuint program = glCreateShader(GL_VERTEX_SHADER);
	// const char *source_c_str = vsource.c_str();
	// glShaderSource(program, 1, &source_c_str, nullptr);
	// glCompileShader(program);
	//
	// printf("program: %d\n", program);
	//
	// int success;
	// char error_log[512];
	// glGetShaderiv(program, GL_COMPILE_STATUS, &success);
	// if (!success) {
	// 	glGetShaderInfoLog(program, 512, NULL, error_log);
	// 	fmt::println("compilation error: {}", error_log);
	// }
}
