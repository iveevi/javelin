#include <array>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "ire/emitter.hpp"
#include "ire/gltype.hpp"
#include "ire/type_synthesis.hpp"

#include "ire/core.hpp"

namespace jvl::ire {

// TODO: layout qualifiers for structs?

template <synthesizable T, synthesizable ... Args>
int argument_list(const T &t, const Args &...args)
{
	auto &em = Emitter::active;

	op::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(Args))
		l.next = argument_list(args...);

	return em.emit(l);
}

template <synthesizable ... Args>
requires (sizeof...(Args) > 0)
void structure(const Args &... args)
{
	auto &em = Emitter::active;

	op::Construct ctor;
	ctor.type = synthesize_type_fields <Args...> ().id;
	ctor.args = argument_list(args...);

	em.emit_main(ctor);
}

// Matrix types
template <typename T, size_t N, size_t M>
struct mat_base : tagged {
	T data[N][M];

	// TODO: operator[]
	cache_index_t synthesize() const {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		// TODO: manual assignment, except for mat2x2
		// TODO: instantiate all primitive types at header
		op::TypeField tf;
		tf.item = op::mat4;

		op::Primitive p;
		p.type = op::i32;
		p.idata[0] = 1;

		op::List l;
		l.item = em.emit(p);

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = em.emit(l);

		return (this->ref = em.emit(ctor));
	}
};

template <typename T, size_t N, size_t M>
struct mat : mat_base <T, N, M> {
	using mat_base <T, N, M> ::mat_base;
};

// Common types
using i32 = gltype <int>;
using f32 = gltype <float>;
using boolean = gltype <bool>;

using ivec2 = vec <int, 2>;
using ivec3 = vec <int, 3>;
using ivec4 = vec <int, 4>;

using vec2 = vec <float, 2>;
using vec3 = vec <float, 3>;
using vec4 = vec <float, 4>;

using mat2 = mat <float, 2, 2>;
using mat3 = mat <float, 3, 3>;
using mat4 = mat <float, 4, 4>;

// Type matching
template <typename T>
constexpr op::PrimitiveType primitive_type()
{
	if constexpr (std::is_same_v <T, bool>)
		return op::boolean;
	if constexpr (std::is_same_v <T, int>)
		return op::i32;
	if constexpr (std::is_same_v <T, float>)
		return op::f32;

	if constexpr (std::is_same_v <T, gltype <bool>>)
		return op::boolean;
	if constexpr (std::is_same_v <T, gltype <int>>)
		return op::i32;
	if constexpr (std::is_same_v <T, gltype <float>>)
		return op::f32;

	if constexpr (std::is_same_v <T, vec2>)
		return op::vec2;
	if constexpr (std::is_same_v <T, vec3>)
		return op::vec3;
	if constexpr (std::is_same_v <T, vec4>)
		return op::vec4;

	if constexpr (std::is_same_v <T, ivec2>)
		return op::ivec2;
	if constexpr (std::is_same_v <T, ivec3>)
		return op::ivec3;
	if constexpr (std::is_same_v <T, ivec4>)
		return op::ivec4;

	if constexpr (std::is_same_v <T, mat2>)
		return op::mat2;
	if constexpr (std::is_same_v <T, mat3>)
		return op::mat3;
	if constexpr (std::is_same_v <T, mat4>)
		return op::mat4;

	return op::bad;
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

	vec2 vec;
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
