#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <ostream>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "ire/op.hpp"
#include "wrapped_types.hpp"

namespace jvl::ire {

// Compressing IR code sequences
using block_atom_t = uint32_t;
constexpr size_t block_size = (sizeof(op::General) + sizeof(block_atom_t) - 1)/sizeof(block_atom_t);
using block_t = std::array <block_atom_t, block_size>;

}

bool operator==(const jvl::ire::block_t &A, const jvl::ire::block_t &B)
{
	for (size_t i = 0; i < jvl::ire::block_size; i++) {
		if (A[i] != B[i])
			return false;
	}

	return true;
}

template <>
struct std::hash <jvl::ire::block_t> {
	size_t operator()(const jvl::ire::block_t &b) const {
		auto h = hash <jvl::ire::block_atom_t> ();

		size_t x = 0;
		for (size_t i = 0; i < jvl::ire::block_size; i++)
			x ^= h(b[i]);

		return x;
	}
};

namespace jvl::ire {

block_t cast_to_block(const op::General &g)
{
	block_t b;
	std::memcpy(b.data(), &g, sizeof(op::General));
	return b;
}

size_t ir_compact_deduplicate(const op::General *const source, op::General *const dst, std::unordered_set <int> &main, size_t elements)
{
	wrapped::hash_table <block_t, int> blocks;
	wrapped::hash_table <int, int> reindex;
	std::unordered_set <int> original;

	// Find duplicates (binary)
	// TODO: exception is if it is main
	for (size_t i = 0; i < elements; i++) {
		block_t b = cast_to_block(source[i]);
		if (blocks.count(b)) {
			reindex[i] = blocks[b];
		} else {
			reindex[i] = blocks.size();
			blocks[b] = blocks.size();
			original.insert(i);
		}
	}

	// Re-synthesize
	size_t size = 0;
	for (size_t i = 0; i < elements; i++) {
		if (original.count(i))
			dst[size++] = source[i];
	}

	// Fix the main instruction indices
	std::unordered_set <int> fixed_main;
	for (int i : main)
		fixed_main.insert(reindex[i]);

	main = fixed_main;

	// Re-index all instructions as necessary
	// TODO: wrap the reindexer that aborts when not found (and -1 -> -1)
	reindex[-1] = -1;

	op::reindex_vd rvd(reindex);
	for (size_t i = 0; i < size; i++) {
		std::visit(rvd, dst[i]);
	}

	return size;

	// TODO: reindex

	// TODO: structure similarity (i.e. lists)
}

struct Emitter {
	// By default the program begins at index=0
	op::General *pool;
	op::General *dual;

	size_t size;
	size_t pointer;

	std::stack <int> control_flow_ends;
	std::unordered_set <int> main;

	Emitter() : pool(nullptr), dual(nullptr), size(0), pointer(0) {}

	void compact() {
		pointer = ir_compact_deduplicate(pool, dual, main, pointer);
		std::memset(pool, 0, size * sizeof(op::General));
		std::memcpy(pool, dual, pointer * sizeof(op::General));
	}

	void resize(size_t units) {
		size_t count = 0;

		// TODO: compact IR before resizing
		op::General *old_pool = pool;
		op::General *old_dual = dual;

		pool = new op::General[units];
		dual = new op::General[units];

		std::memset(pool, 0, units * sizeof(op::General));
		std::memset(dual, 0, units * sizeof(op::General));

		if (old_pool) {
			std::memcpy(pool, old_pool, size * sizeof(op::General));
			delete[] old_pool;
		}

		if (old_dual) {
			std::memcpy(dual, old_dual, size * sizeof(op::General));
			delete[] old_dual;
		}

		size = units;
	}

	// Emitting instructions during function invocation
	int emit(const op::General &op) {
		if (pointer >= size) {
			if (size == 0)
				resize(1 << 4);
			else
				resize(size << 2);
		}

		if (pointer >= size) {
			printf("error, exceed global pool size, please reserve beforehand\n");
			throw -1;
			return -1;
		}

		pool[pointer] = op;
		return pointer++;
	}

	int emit_main(const op::General &op) {
		int p = emit(op);
		main.insert(p);
		return p;
	}

	int emit_main(const op::Cond &cond) {
		int p = emit_main((op::General) cond);
		control_flow_ends.push(p);
		return p;
	}

	int emit_main(const op::Elif &elif) {
		int p = emit_main((op::General) elif);
		assert(control_flow_ends.size());
		auto ref = control_flow_ends.top();
		control_flow_ends.pop();
		control_flow_callback(ref, p);
		control_flow_ends.push(p);
		return p;
	}

	int emit_main(const op::End &end) {
		int p = emit_main((op::General) end);
		assert(control_flow_ends.size());
		auto ref = control_flow_ends.top();
		control_flow_ends.pop();
		control_flow_callback(ref, p);
		return p;
	}

	void control_flow_callback(int ref, int p) {
		auto &op = pool[ref];
		if (std::holds_alternative <op::Cond> (op)) {
			std::get <op::Cond> (op).failto = p;
		} else if (std::holds_alternative <op::Elif> (op)) {
			std::get <op::Elif> (op).failto = p;
		} else {
			printf("op not conditional, is actually:");
			std::visit(op::dump_vd(), op);
			printf("\n");
			assert(false);
		}
	}

	// Generating GLSL source code
	std::string generate_glsl() {
		// TODO: mark instructions as unused (unless flags are given)

		// Final generated source
		std::string source;

		// Version header
		source += "#version 450\n";
		source += "\n";

		// Gather all necessary structs
		wrapped::hash_table <int, std::string> struct_names;

		int struct_index = 0;
		for (int i = 0; i < pointer; i++) {
			op::General g = pool[i];
			if (!g.is <op::TypeField> ())
				continue;

			// TODO: skip if unused as well

			op::TypeField tf = g.as <op::TypeField> ();
			// TODO: skip if its only one primitive
			if (tf.next == -1 && tf.down == -1)
				continue;

			// TODO: handle nested structs (down)

			std::string struct_name = fmt::format("s{}_t", struct_index++);
			struct_names[i] = struct_name;

			std::string struct_source;
			struct_source = fmt::format("struct {} {{\n", struct_name);

			int field_index = 0;
			int j = i;
			while (j != -1) {
				op::General g = pool[j];
				if (!g.is <op::TypeField> ())
					abort();

				op::TypeField tf = g.as <op::TypeField> ();
				// TODO: nested
				// TODO: put this whole thing in a method

				struct_source += fmt::format("    {} f{};\n",
						op::type_table[tf.item],
						field_index++);

				j = tf.next;
			}

			struct_source += "};\n\n";

			source += struct_source;
		}

		// Gather all global shader variables
		std::map <int, std::string> lins;
		std::map <int, std::string> louts;

		for (int i = 0; i < pointer; i++) {
			auto op = pool[i];
			if (!std::holds_alternative <op::Global> (op))
				continue;

			auto global = std::get <op::Global> (op);
			auto type = std::visit(op::typeof_vd(pool, struct_names), op);
			if (global.qualifier == op::Global::layout_in)
				lins[global.binding] = type;
			else if (global.qualifier == op::Global::layout_out)
				louts[global.binding] = type;
		}

		// Actual translation
		op::translate_glsl_vd tdisp;
		tdisp.pool = pool;
		tdisp.struct_names = struct_names;

		// Global shader variables
		// TODO: check for vulkan target, etc
		for (const auto &[binding, type] : lins)
			source += fmt::format("layout (location = {}) in {} _lin{};\n", binding, type, binding);
		source += "\n";

		// TODO: remove extra space
		for (const auto &[binding, type] : louts)
			source += fmt::format("layout (location = {}) out {} _lout{};\n", binding, type, binding);
		source += "\n";

		// Main function
		source += "void main()\n";
		source += "{\n";
		for (int i : main)
			source += "    " + tdisp.eval(i);
		source += "}\n";

		return source;
	}

	// Printing the IR state
	// TODO: debug only?
	void dump() {
		printf("GLOBALS (%4d/%4d)\n", pointer, size);
		for (size_t i = 0; i < pointer; i++) {
			if (std::ranges::find(main.begin(), main.end(), i) != main.end())
				printf("[*] ");
			else
				printf("    ");

			printf("[%4d]: ", i);
			std::visit(op::dump_vd(), pool[i]);
			printf("\n");
		}
	}

	// TODO: one per thread
	static thread_local Emitter active;
};

thread_local Emitter Emitter::active;

// Interface
struct emit_index_t {
	using value_type = int;

	value_type id;

	emit_index_t &operator=(const value_type &v) {
		id = v;
		return *this;
	}

	static emit_index_t null() {
		return { .id = -1 };
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

	[[gnu::always_inline]]
	bool cached() const {
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
	{ t.synthesize() } -> std::same_as <emit_index_t>;
};

template <typename T, typename ... Args>
concept callbacked = requires(T &t, const Args &...args) {
	{ t.callback(args...) };
};

// Forward declarations
template <typename T, size_t binding>
// TODO: tagging
struct layout_in : tagged {
	emit_index_t synthesize() const {
		if (cached())
			return ref;

		auto &em = Emitter::active;

		// TODO: type_match t construct a TypeField
		op::TypeField type;
		type.item = type_match <T> ();
		type.next = -1;

		op::Global qualifier;
		qualifier.type = em.emit(type);
		qualifier.binding = binding;
		qualifier.qualifier = op::Global::layout_in;

		op::Load load;
		load.src = em.emit(qualifier);

		return (ref = em.emit_main(load));
	}

	operator T() {
		return T(synthesize());
	}
};

template <typename T, size_t binding>
requires synthesizable <T>
struct layout_out : T {
	void operator=(const T &t) const {
		auto &em = Emitter::active;

		op::TypeField type;
		type.item = type_match <T> ();
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

	vec(const vec <T, 3> &v, const float &x) {
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
requires synthesizable <T> && synthesizable <U>
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
requires synthesizable <T>
boolean operator==(const T &A, const U &B)
{
	return A == gltype <U> (B);
}

template <typename T, typename U>
requires synthesizable <U>
boolean operator==(const T &A, const U &B)
{
	return gltype <T> (A) == B;
}

// Branching emitters
void cond(boolean b)
{
	auto &em = Emitter::active;
	op::Cond branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

void elif(boolean b)
{
	auto &em = Emitter::active;
	op::Elif branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

void elif()
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

template <typename T, typename ...Args>
static emit_index_t synthesize_type_field() {
	static thread_local emit_index_t cached = emit_index_t::null();
	if (cached.id != -1)
		return cached;

	auto &em = Emitter::active;

	op::TypeField tf;
	tf.item = type_match <T> ();
	if constexpr (sizeof...(Args))
		tf.next = synthesize_type_field <Args...> ().id;
	else
		tf.next = -1;

	return (cached = em.emit(tf));
}

template <typename T, typename ...Args>
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

template <typename ...Args>
requires (synthesizable <Args> && ...)
struct structure {
	structure(const Args &... args) {
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

static_assert(synthesizable <gltype <int>>);

static_assert(synthesizable <boolean>);

static_assert(synthesizable <layout_in <int, 0>>);

static_assert(synthesizable <vec2>);
static_assert(synthesizable <vec3>);
static_assert(synthesizable <vec4>);

static_assert(synthesizable <mat4>);

}

using namespace jvl::ire;

struct mvp : structure <mat4, mat4, mat4> {
	mat4 model;
	mat4 view;
	mat4 proj;

	using structure <mat4, mat4, mat4> ::structure;
};

void vertex_shader()
{
	layout_in <vec3, 0> position;
	mvp mvp { mat4(), mat4(), mat4() };

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

	Emitter::active.dump();
	Emitter::active.compact();
	Emitter::active.dump();

	auto source = Emitter::active.generate_glsl();

	printf("\nGLSL:\n%s", source.c_str());

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(800, 800, "Window", NULL, NULL);
	if (window == NULL)
	{
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
