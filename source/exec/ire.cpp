#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "ire/core.hpp"
#include "ire/tagged.hpp"
#include "ire/uniform_layout.hpp"
#include "ire/vector.hpp"

namespace jvl::ire {

// TODO: with name
template <synthesizable ... Args>
requires (sizeof...(Args) > 0)
void structure(const Args &... args)
{
	auto &em = Emitter::active;

	atom::Construct ctor;
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

	atom::Cmp cmp;
	cmp.a = a;
	cmp.b = b;
	cmp.type = atom::Cmp::eq;

	cache_index_t c;
	c = em.emit(cmp);

	return boolean(c);
}

// Promoting to gltype
template <synthesizable T, typename U>
boolean operator==(const T &A, const U &B)
{
	return A == primitive_t <U> (B);
}

template <typename T, synthesizable U>
boolean operator==(const T &A, const U &B)
{
	return primitive_t <T> (A) == B;
}

// TODO: std.hpp for additional features

} // namespace jvl::ire

using namespace jvl::ire;

void shader()
{
	struct mvp {
		mat4 model;
		mat4 view;
		mat4 proj;

		auto layout() {
			return uniform_layout(model, view, proj);
		}
	};

	layout_in <vec3, 0> position;

	push_constant <mvp> mvp;

	// gl_Position = mvp.proj * vec4()
	gl_Position = vec4(position, 0);
	gl_Position.y = -gl_Position.y;

	// TODO: immutability for shader inputs
	// TODO: before synthesis, demote variables to inline if they are not modified later
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
	Emitter::active.validate();

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
