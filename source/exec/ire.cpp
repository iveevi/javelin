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

// Branching emitters
// TODO: control flow header
void cond(boolean b)
{
	auto &em = Emitter::active;
	op::Cond branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

void elif (boolean b)
{
	auto &em = Emitter::active;
	op::Elif branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

void elif ()
{
	// Treated as an else
	auto &em = Emitter::active;
	op::Elif branch;
	branch.cond = -1;
	em.emit_main(branch);
}

void end()
{
	auto &em = Emitter::active;
	em.emit_main(op::End());
}

// TODO: core.hpp header for main glsl functionality
// then std.hpp for additional features

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
	// layout_in <mvp_info, 0> flag;
	push_constants <mvp_info> flag;

	f32 v = flag.scalar;
	v = 1;

	vec4 vec(2, 0, 1);
	v = vec.y;

	// f32 x = 2.335f;
	// cond(flag.model == 0);
	// 	x = 1.618f;
	// elif();
	// 	x = 0.618f;
	// end();
	//
	// layout_out <float, 0> out;
	//
	// out = x;
	// out = 1.0f;

	// TODO: warnings for the unused sections

	// layout_in <vec3, 0> position;
	//
	// push_constants <mvp_info> mvp;
	//
	// vec4 v = vec4(mvp.camera, 1);
	//
	// f32 x = 2.335f;
	// x = mvp.scalar;
	// vec4 w = vec4(mvp.camera, x);
}

#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	shader();

	// Emitter::active.dump();
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
