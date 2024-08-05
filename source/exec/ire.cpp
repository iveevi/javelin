#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "ire/op.hpp"
#include "ire/emitter.hpp"

namespace jvl::ire {

// Interface
struct emit_index_t {
	using value_type = int;

	value_type id;

	emit_index_t &operator=(const value_type &v) {
		id = v;
		return *this;
	}

	static emit_index_t null() {
		return {.id = -1};
	}

	bool operator==(const value_type &v) {
		return id == v;
	}

	bool operator==(const emit_index_t &c) {
		return id == c.id;
	}
};

struct tagged {
	mutable emit_index_t ref;

	tagged(emit_index_t r = emit_index_t::null()) : ref(r) {}

	[[gnu::always_inline]] bool cached() const {
		return ref != -1;
	}
};

// Way to upcast C++ primitives into a workable type
template <typename T>
struct gltype {};

template <typename T>
constexpr auto type_match();

template <typename T>
concept synthesizable = requires(const T &t) {
	{
		t.synthesize()
	} -> std::same_as<emit_index_t>;
};

template <typename T, typename... Args>
concept callbacked = requires(T &t, const Args &...args) {
	{
		t.callback(args...)
	};
};

// Forward declarations
template <typename T, size_t binding>
// TODO: tagging
struct layout_in : tagged {
	emit_index_t synthesize() const
	{
		if (cached())
			return ref;

		auto &em = Emitter::active;

		// TODO: type_match t construct a TypeField
		op::TypeField type;
		type.item = type_match<T>();
		type.next = -1;

		op::Global qualifier;
		qualifier.type = em.emit(type);
		qualifier.binding = binding;
		qualifier.qualifier = op::Global::layout_in;

		op::Load load;
		load.src = em.emit(qualifier);

		return (ref = em.emit_main(load));
	}

	operator T() { return T(synthesize()); }
};

template <typename T, size_t binding>
	requires synthesizable<T>
struct layout_out : T {
	void operator=(const T &t) const
	{
		auto &em = Emitter::active;

		op::TypeField type;
		type.item = type_match<T>();
		type.next = -1;

		op::Global qualifier;
		qualifier.type = em.emit(type);
		qualifier.binding = binding;
		qualifier.qualifier = op::Global::layout_out;

		op::Store store;
		store.dst = em.emit(qualifier);
		store.src = t.synthesize().id;

		em.emit_main(store);
	}
};

template <typename T, typename Up, typename P, P payload>
// requires callbacked <Up>
struct phantom_type;

template <typename T, size_t N>
struct swizzle_base : tagged {};

// TODO: inherit from base 3 and add extras
template <typename T>
struct swizzle_base<T, 4> : tagged {
	using tagged::tagged;

	using payload = op::Swizzle;

	template <payload p>
	using phantom = phantom_type<T, swizzle_base<T, 4>, payload, p>;

	void callback(const payload &payload) { printf("callback!!\n"); }

	phantom<payload::x> x;
	phantom<payload::y> y;
	phantom<payload::z> z;
	phantom<payload::w> w;
};

template <typename T, typename Up, typename P, P payload>
struct phantom_type {
	T operator=(const T &v)
	{
		printf("assigned to!\n");
		((Up *) this)->callback(payload);
		return v;
	}
};

template <typename T, size_t N>
struct vec_base : swizzle_base<T, N> {
	T data[N];

	constexpr vec_base() = default;

	constexpr vec_base(emit_index_t r) : swizzle_base<T, N>(r) {}

	constexpr vec_base(T v)
	{
		for (size_t i = 0; i < N; i++)
			data[i] = v;
	}

	emit_index_t synthesize() const
	{
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		op::Primitive p;
		// TODO: fix
		p.type = op::vec4;
		std::memcpy(p.fdata, data, N * sizeof(T));

		return (this->ref = em.emit(p));
	}
};

template <typename T, size_t N>
struct vec : vec_base<T, N> {
	using vec_base<T, N>::vec_base;
};

template <typename T>
struct vec<T, 4> : vec_base<T, 4> {
	using vec_base<T, 4>::vec_base;

	vec(const vec<T, 3> &v, const float &x)
	{
		auto &em = Emitter::active;

		// TODO: conversion (f32 struct)
		op::Primitive px;
		px.type = op::f32;
		px.fdata[0] = x;

		op::List lx;
		lx.item = em.emit(px);
		lx.next = -1;

		op::List lv;
		lv.item = v.synthesize().id;
		lv.next = em.emit(lx);

		op::TypeField type;
		type.item = op::vec4;
		type.next = -1;

		op::Construct ctor;
		ctor.type = em.emit(type);
		ctor.args = em.emit(lv);

		this->ref = em.emit_main(ctor);
	}
};

template <typename T, size_t N, size_t M>
struct mat_base : tagged {
	T data[N][M];

	// TODO: operator[]
	emit_index_t synthesize() const {
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

template <>
struct gltype <int> : tagged {
	int value;

	gltype(int v = 0) : value(v) {}

	emit_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		op::Primitive p;
		p.type = op::i32;
		p.idata[0] = value;

		return (ref = em.emit(p));
	}
};

template <>
struct gltype <bool> : tagged {
	using tagged::tagged;

	bool value;

	emit_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		op::Primitive t;
		t.type = op::PrimitiveType::boolean;

		return (ref = em.emit(t));
	}
};

// Common types
using boolean = gltype <bool>;

using vec2 = vec <float, 2>;
using vec3 = vec <float, 3>;
using vec4 = vec <float, 4>;

using mat3 = mat <float, 3, 3>;
using mat4 = mat <float, 4, 4>;

// Type matching
template <typename T>
constexpr auto type_match()
{
	if constexpr (std::is_same_v <T, bool>)
		return op::PrimitiveType::boolean;
	if constexpr (std::is_same_v <T, int>)
		return op::PrimitiveType::i32;
	if constexpr (std::is_same_v <T, float>)
		return op::PrimitiveType::f32;
	if constexpr (std::is_same_v <T, vec2>)
		return op::PrimitiveType::vec2;
	if constexpr (std::is_same_v <T, vec3>)
		return op::PrimitiveType::vec3;
	if constexpr (std::is_same_v <T, vec4>)
		return op::PrimitiveType::vec4;
	if constexpr (std::is_same_v <T, mat4>)
		return op::PrimitiveType::mat4;

	return op::PrimitiveType::bad;
}

template <typename T, typename U>
	requires synthesizable<T> && synthesizable<U>
boolean operator==(const T &A, const U &B)
{
	auto &em = Emitter::active;

	int a = A.synthesize().id;
	int b = B.synthesize().id;

	op::Cmp cmp;
	cmp.a = a;
	cmp.b = b;
	cmp.type = op::Cmp::eq;

	emit_index_t c;
	c = em.emit(cmp);

	return boolean(c);
}

// Promoting to gltype
template <typename T, typename U>
	requires synthesizable<T>
boolean operator==(const T &A, const U &B)
{
	return A == gltype<U>(B);
}

template <typename T, typename U>
	requires synthesizable<U>
boolean operator==(const T &A, const U &B)
{
	return gltype<T>(A) == B;
}

// Branching emitters
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

struct push_constants {};

template <typename T, typename... Args>
static emit_index_t synthesize_type_field()
{
	static thread_local emit_index_t cached = emit_index_t::null();
	if (cached.id != -1)
		return cached;

	auto &em = Emitter::active;

	op::TypeField tf;
	tf.item = type_match<T>();
	if constexpr (sizeof...(Args))
		tf.next = synthesize_type_field<Args...>().id;
	else
		tf.next = -1;

	return (cached = em.emit(tf));
}

template <typename T, typename... Args>
requires synthesizable <T> && (synthesizable <Args> && ...)
int argument_list(const T &t, const Args &...args)
{
	auto &em = Emitter::active;

	op::List l;
	l.item = t.synthesize().id;

	if constexpr (sizeof...(Args))
		l.next = argument_list(args...);

	return em.emit(l);
}

struct structure {
	template <typename... Args>
	requires(synthesizable <Args> && ...)
	structure(const Args &...args) {
		auto &em = Emitter::active;

		op::Construct ctor;
		ctor.type = synthesize_type_field <Args...> ().id;
		ctor.args = argument_list(args...);

		em.emit_main(ctor);
	}
};

// Guarantees
// TODO: debug only
static_assert(synthesizable <vec4>);

static_assert(synthesizable <gltype<int>>);

static_assert(synthesizable <boolean>);

static_assert(synthesizable <layout_in<int, 0>>);

static_assert(synthesizable <vec2>);
static_assert(synthesizable <vec3>);
static_assert(synthesizable <vec4>);

static_assert(synthesizable <mat4>);

} // namespace jvl::ire

using namespace jvl::ire;

struct mvp : structure {
	mat4 model;
	mat4 view;
	mat4 proj;

	using structure::structure;
	// mvp() : structure(model, view, proj) {}
	// using structure <mat4, mat4, mat4> ::structure;
};

void vertex_shader()
{
	layout_in <vec3, 0> position;
	mvp mvp { mat4(), mat4(), mat4() };
	// mvp mvp;

	// push_constants <mvp> mvp;

	vec4 v = vec4(position, 1);
}

void fragment_shader()
{
	layout_in <int, 0> flag;
	layout_out <vec4, 0> fragment;

	fragment = vec4(1.0);
	fragment.x = 1.0f;
	// cond(flag == 0);
	// 	fragment = vec4(1.0);
	// elif(flag == 1);
	// 	fragment = vec4(0.5);
	// elif();
	// 	fragment = vec4(0.1);
	// end();
}

#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	vertex_shader();

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
