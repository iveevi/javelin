#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "ire/atom.hpp"
#include "ire/core.hpp"
#include "ire/tagged.hpp"
#include "ire/uniform_layout.hpp"
#include "ire/util.hpp"
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

void vertex_shader()
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
	layout_out <vec3, 0> out_position;

	push_constant <mvp> mvp;

	vec4 v = vec4(position, 1);
	gl_Position = mvp.proj * (mvp.view * (mvp.model * v));
	gl_Position.y = -gl_Position.y;
	out_position = position;

	// TODO: immutability for shader inputs
	// TODO: before synthesis, demote variables to inline if they are not modified later
	// TODO: warnings for the unused sections
}

template <typename T, size_t N>
vec <T, N> dFdx(const vec <T, N> &v)
{
	static const char name[] = "dFdx";

	auto &em = Emitter::active;

	atom::Intrinsic intr;
	intr.name = name;
	intr.args = synthesize_list(v);
	intr.ret = synthesize_type_fields <vec <T, N>> ().id;

	cache_index_t cit;
	cit = em.emit_main(intr);

	return cit;
}

template <typename T, size_t N>
vec <T, N> dFdy(const vec <T, N> &v)
{
	static const char name[] = "dFdy";

	auto &em = Emitter::active;

	atom::Intrinsic intr;
	intr.name = name;
	intr.args = synthesize_list(v);
	intr.ret = synthesize_type_fields <vec <T, N>> ().id;

	cache_index_t cit;
	cit = em.emit_main(intr);

	return cit;
}

template <typename T>
vec <T, 3> cross(const vec <T, 3> &v, const vec <T, 3> &w)
{
	static const char name[] = "cross";

	auto &em = Emitter::active;

	atom::Intrinsic intr;
	intr.name = name;
	intr.args = synthesize_list(v, w);
	intr.ret = synthesize_type_fields <vec <T, 3>> ().id;

	cache_index_t cit;
	cit = em.emit_main(intr);

	return cit;
}

template <typename T, size_t N>
vec <T, N> normalize(const vec <T, N> &v)
{
	static const char name[] = "normalize";

	auto &em = Emitter::active;

	atom::Intrinsic intr;
	intr.name = name;
	intr.args = synthesize_list(v);
	intr.ret = synthesize_type_fields <vec <T, N>> ().id;

	cache_index_t cit;
	cit = em.emit_main(intr);

	return cit;
}

void fragment_shader()
{
	layout_in <vec3, 0> position;
	layout_out <vec4, 0> fragment;

	vec3 dU = dFdx(position);
	vec3 dV = dFdy(position);
	vec3 N = normalize(cross(dU, dV));
	fragment = vec4(0.5f + 0.5f * N, 1);
}

#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	Emitter::active.clear();

	vertex_shader();

	// TODO: auto kernel = jvl_emit_kernel(ftn, args...)
	//            ^
	//            now kernel holds the IR;
	//            operator()(...) semantics to be dealt
	//            with later...
	Emitter::active.compact();
	Emitter::active.dump();
	Emitter::active.validate();

	auto vsource = Emitter::active.generate_glsl();

	printf("\nvertex shader:\n%s", vsource.c_str());

	Emitter::active.clear();

	fragment_shader();

	Emitter::active.compact();
	Emitter::active.dump();
	Emitter::active.validate();

	auto fsource = Emitter::active.generate_glsl();

	printf("\nfragment shader:\n%s", fsource.c_str());

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
	const char *source_c_str = vsource.c_str();
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
