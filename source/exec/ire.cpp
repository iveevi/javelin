#include <array>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "ire/core.hpp"

namespace jvl::ire {

template <synthesizable ... Args>
requires (sizeof...(Args) > 0)
void structure(const Args &... args)
{
	auto &em = Emitter::active;

	op::Construct ctor;
	ctor.type = synthesize_type_fields <Args...> ().id;
	ctor.args = synthesize_list(args...);

	em.emit_main(ctor);
}

template <synthesizable T, synthesizable U>
boolean operator==(const T &A, const U &B)
{
	auto &em = Emitter::active;

	int a = A.synthesize().id;
	int b = B.synthesize().id;

	op::Cmp cmp;
	cmp.a = a;
	cmp.b = b;
	cmp.type = op::Cmp::eq;

	cache_index_t c;
	c = em.emit(cmp);

	return boolean(c);
}

// Promoting to gltype
template <synthesizable T, typename U>
boolean operator==(const T &A, const U &B)
{
	return A == gltype <U> (B);
}

template <typename T, synthesizable U>
boolean operator==(const T &A, const U &B)
{
	return gltype <T> (A) == B;
}

// TODO: std.hpp for additional features

} // namespace jvl::ire

using namespace jvl::ire;

// TODO: nested structs
struct mvp_info {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 camera;
	f32 scalar;

	// TODO: simultaenously a struct and push constant
	// mvp_info(bool stationary = false) {
	// 	if (stationary)
	// 		structure(view, proj);
	// 	else
	// 		structure(model, view, proj);
	// }

	auto layout() {
		return uniform_layout(model, view, proj, camera, scalar);
	}
};

void shader()
{
	// push_constants <mvp_info> flag;
	//
	// f32 v = flag.scalar;
	// v = 1;
	//
	// vec4 vec(2, 0, 1);
	// v = vec.y;

	f32 v = 0;

	// TODO: before synthesis, demote variables to inline if they are not modified later
	f32 i = 0;
	loop(i == 0);
		v = i;
	end();

	// TODO: warnings for the unused sections
}

#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	shader();

	Emitter::active.compact();
	Emitter::active.dump();

	auto source = Emitter::active.generate_glsl();

	printf("\nGLSL:\n%s", source.c_str());

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(800, 800, "Window", NULL, NULL);
	if (window == NULL) {
		printf("failed to load glfw\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
		printf("failed to load glad\n");
		return -1;
	}

	GLuint program = glCreateShader(GL_VERTEX_SHADER);
	const char *source_c_str = source.c_str();
	glShaderSource(program, 1, &source_c_str, nullptr);
	glCompileShader(program);

	printf("program: %d\n", program);

	int success;
	char error_log[512];
	glGetShaderiv(program, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(program, 512, NULL, error_log);
		fmt::println("compilation error: {}", error_log);
	}
}
