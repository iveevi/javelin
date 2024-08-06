#include <array>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "ire/op.hpp"
#include "ire/emitter.hpp"
#include "ire/gltype.hpp"
#include "ire/tagged.hpp"

namespace jvl::ire {

template <typename T>
constexpr op::PrimitiveType primitive_type();

template <typename T>
int type_field()
{
	auto &em = Emitter::active;

	// TODO: for all types
	op::TypeField type;
	type.item = primitive_type <T> ();
	type.next = -1;

	return em.emit(type);
}

template <typename T>
concept synthesizable = requires(const T &t) {
	{
		t.synthesize()
	} -> std::same_as <emit_index_t>;
};

template <typename T, typename ... Args>
concept callbacked = requires(T &t, const Args &...args) {
	{
		t.callback(args...)
	};
};

struct __uniform_layout_key {};

template <typename ... Args>
requires (sizeof...(Args) > 0)
struct uniform_layout {
	std::array <tagged *, sizeof...(Args)> fields;

	template <typename T, typename ... UArgs>
	[[gnu::always_inline]]
	void __init(int index, T &t, UArgs &... uargs) {
		// TODO: how to deal with nested structs?
		fields[index] = &static_cast <tagged &> (t);
		if constexpr (sizeof...(uargs) > 0)
			__init(index + 1, uargs...);
	}

	uniform_layout(Args &... args) {
		__init(0, args...);
	}

	__uniform_layout_key key() const {
		return {};
	}
};

template <typename T>
concept uniform_compatible = requires {
	{
		T().layout().key()
	} -> std::same_as <__uniform_layout_key>;
};

// Forward declarations
template <typename T, size_t binding>
struct layout_in : tagged {
	emit_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		op::Global global;
		global.type = type_field <T> ();
		global.binding = binding;
		global.qualifier = op::Global::layout_in;

		op::Load load;
		load.src = em.emit(global);

		return (ref = em.emit_main(load));
	}

	operator T() {
		return T(synthesize());
	}
};

template <synthesizable T, size_t binding>
struct layout_out : T {
	void operator=(const T &t) const {
		auto &em = Emitter::active;

		op::Global global;
		global.type = type_field <T> ();
		global.binding = binding;
		global.qualifier = op::Global::layout_out;

		op::Store store;
		store.dst = em.emit(global);
		store.src = t.synthesize().id;

		em.emit_main(store);
	}
};

// TODO: layout qualifiers for structs?

template <typename T, typename ... Args>
emit_index_t synthesize_type_fields()
{
	static thread_local emit_index_t cached = emit_index_t::null();
	if (cached.id != -1)
		return cached;

	auto &em = Emitter::active;

	op::TypeField tf;
	tf.item = primitive_type <T> ();
	if constexpr (sizeof...(Args))
		tf.next = synthesize_type_fields <Args...> ().id;
	else
		tf.next = -1;

	return (cached = em.emit(tf));
}

template <typename ... Args>
emit_index_t synthesize_type_fields(const uniform_layout <Args...> &args)
{
	return synthesize_type_fields <Args...> ();
}

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

template <uniform_compatible T>
struct push_constants : T {
	emit_index_t ref = emit_index_t::null();

	// TODO: handle operator= disabling

	template <typename ... Args>
	push_constants(const Args &... args) : T(args...) {
		auto &em = Emitter::active;

		auto uniform_layout = this->layout();

		op::Global global;
		global.type = synthesize_type_fields(uniform_layout).id;
		global.binding = -1;
		global.qualifier = op::Global::push_constant;

		ref = em.emit(global);

		for (size_t i = 0; i < uniform_layout.fields.size(); i++) {
			op::Load load;
			load.src = ref.id;
			load.idx = i;
			uniform_layout.fields[i]->ref = em.emit(load);
		}

		// TODO: uncached type which clears each time
	}
};

template <typename T, typename Up, typename P, P payload>
struct phantom_type;

template <typename T, size_t N>
struct swizzle_base : tagged {};

// TODO: inherit from base 3 and add extras
template <typename T>
struct swizzle_base <T, 4> : tagged {
	using tagged::tagged;

	using payload = op::Swizzle;

	template <payload p>
	using phantom = phantom_type <T, swizzle_base <T, 4>, payload, p>;

	void callback(const payload &payload) {
		printf("callback!!\n");
	}

	phantom <payload::x> x;
	phantom <payload::y> y;
	phantom <payload::z> z;
	phantom <payload::w> w;
};

template <typename T, typename Up, typename P, P payload>
struct phantom_type {
	// TODO: return a gltype
	T operator=(const T &v) {
		printf("assigned to!\n");
		((Up *) this)->callback(payload);
		return v;
	}
};

template <typename T, size_t N>
struct vec_base : swizzle_base <T, N> {
	T data[N];

	constexpr vec_base() = default;

	constexpr vec_base(emit_index_t r) : swizzle_base <T, N> (r) {}

	constexpr vec_base(T v) {
		for (size_t i = 0; i < N; i++)
			data[i] = v;
	}

	emit_index_t synthesize() const {
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
struct vec : vec_base <T, N> {
	using vec_base <T, N> ::vec_base;
};

template <typename T>
struct vec <T, 4> : vec_base <T, 4> {
	using vec_base <T, 4> ::vec_base;

	vec(const vec <T, 3> &v, const gltype <T> &x) {
		auto &em = Emitter::active;

		op::List lx;
		lx.item = x.synthesize().id;
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

// Common types
using i32 = gltype <int>;
using f32 = gltype <float>;
using boolean = gltype <bool>;

using vec2 = vec <float, 2>;
using vec3 = vec <float, 3>;
using vec4 = vec <float, 4>;

using mat3 = mat <float, 3, 3>;
using mat4 = mat <float, 4, 4>;

// Type matching
template <typename T>
constexpr op::PrimitiveType primitive_type()
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

	emit_index_t c;
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

// Guarantees
// TODO: debug only
static_assert(synthesizable <vec4>);

static_assert(synthesizable <gltype <int>>);

static_assert(synthesizable <boolean>);

static_assert(synthesizable <layout_in <int, 0>>);

static_assert(synthesizable <vec2>);
static_assert(synthesizable <vec3>);
static_assert(synthesizable <vec4>);

static_assert(synthesizable <mat4>);

} // namespace jvl::ire

using namespace jvl::ire;

// TODO: nested structs
struct mvp_info {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 camera;

	// TODO: simultaenously a struct and push constant
	// mvp_info(bool stationary = false) {
	// 	if (stationary)
	// 		structure(view, proj);
	// 	else
	// 		structure(model, view, proj);
	// }

	auto layout() {
		return uniform_layout(model, view, proj, camera);
	}
};

void vertex_shader()
{
	layout_in <vec3, 0> position;

	push_constants <mvp_info> mvp;

	vec4 v = vec4(mvp.camera, 1);

	f32 x = 2.335f;
	vec4 w = vec4(mvp.camera, x);
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
