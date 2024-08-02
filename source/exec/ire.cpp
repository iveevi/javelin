#include <cstdio>
#include <cstring>
#include <functional>
#include <type_traits>
#include <variant>
#include <stack>
#include <vector>
#include <cassert>
#include <map>
#include <string>

#include <fmt/format.h>

namespace op {

struct General;

struct Global {
	int type;
	int binding;

	enum {
		layout_in,
		layout_out
	} qualifier;
};

enum PrimitiveType {
	bad,
	boolean,
	i32,
	f32,
	vec4,
};

struct TypeField {
	// If the current field is a
	// primitive, then item != BAD
	PrimitiveType item;

	// Otherwise it is a nested struct
	int down;

	// Next field
	int next;
};

struct Primitive {
	PrimitiveType type;
	union {
		bool b;
		int idata[4];
		float fdata[4];
	};
};

struct Swizzle {
	enum Kind {
		x, y, z, w,
		xy
	} type;

	constexpr Swizzle(Kind t) : type(t) {}
};

struct Cmp {
	int a;
	int b;
	enum {
		eq, neq,
	} type;
};

struct List {
	int item;
	int next;
};

struct Construct {
	int type;
	int args;
};

struct Store {
	int dst;
	int src;
};

struct Load {
	int src;
};

struct Cond {
	int cond;
	int failto;
};

struct Elif : Cond {};

struct End {};

using _general_base = std::variant <
	Global,
	TypeField,
	Primitive,
	Swizzle,
	Cmp,
	Construct,
	List,
	Store,
	Load,
	Cond,
	Elif,
	End
>;

struct General : _general_base {
	using _general_base::_general_base;
};

// Type translation
static const char *type_table[] = {
	"<BAD>", "bool", "int", "float", "vec4"
};

struct _dump_dispatcher {
	const char* resolve(const PrimitiveType &t) {
		return type_table[t];
	}

	void operator()(const Global &global) {
		static const char *qualifier_table[] = {
			"layout input",
			"layout output"
		};

		printf("global: %%%d = (%s, %d)", global.type,
			qualifier_table[global.qualifier], global.binding);
	}

	void operator()(const TypeField &t) {
		printf("type: ");
		if (t.item != bad)
			printf("%s", resolve(t.item));
		else
			printf("%d", t.down);

		printf(" -> ");
		if (t.next >= 0)
			printf("%d", t.next);
		else
			printf("(nil)");
	}

	void operator()(const Primitive &p) {
		printf("primitive: %s = ", resolve(p.type));

		switch (p.type) {
		case i32:
			printf("%d", p.idata[0]);
			break;
		case vec4:
			printf("(%.2f, %.2f, %.2f, %.2f)",
					p.fdata[0],
					p.fdata[1],
					p.fdata[2],
					p.fdata[3]);
			break;
		default:
			printf("?");
			break;
		}
	}

	void operator()(const List &list) {
		printf("list: %%%d -> ", list.item);
		if (list.next >= 0)
			printf("%%%d", list.next);
		else
			printf("(nil)");
	}

	void operator()(const Construct &ctor) {
		printf("construct: %%%d = %%%d", ctor.type, ctor.args);
	}

	void operator()(const Store &store) {
		printf("store %%%d -> %%%d", store.src, store.dst);
	}

	void operator()(const Load &load) {
		printf("load %%%d", load.src);
	}

	void operator()(const Cmp &cmp) {
		const char *cmp_table[] = { "=", "!=" };
		printf("cmp %%%d %s %%%d", cmp.a, cmp_table[cmp.type], cmp.b);
	}

	void operator()(const Cond &cond) {
		printf("cond %%%d -> %%%d", cond.cond, cond.failto);
	}

	void operator()(const Elif &elif) {
		if (elif.cond >= 0)
			printf("elif %%%d -> %%%d", elif.cond, elif.failto);
		else
			printf("elif (nil) -> %%%d", elif.failto);
	}

	void operator()(const End &) {
		printf("end");
	}

	template <typename T>
	void operator()(const T &) {
		printf("<?>");
	}
};

struct _typeof_dispatcher {
	op::General *pool;

	// TODO: cache system here as well

	// TODO: needs to be persistent handle structs later
	const char *resolve(const PrimitiveType type) {
		return type_table[type];
	}

	std::string operator()(const TypeField &type) {
		if (type.item != bad)
			return resolve(type.item);
		return "<?>";
	}

	std::string operator()(const Global &global) {
		return defer(global.type);
	}

	template <typename T>
	std::string operator()(const T &) {
		return "<?>";
	}

	std::string defer(int index) {
		return std::visit(*this, pool[index]);
	}
};

struct _translate_dispatcher {
	op::General *pool;
	int generator = 0;
	int indentation = 0;
	int next_indentation = 0;
	bool inlining = false;
	std::map <int, std::string> symbols;

	std::string operator()(const Cond &cond) {
		next_indentation++;
		std::string c = defer(cond.cond, true);
		return "if (" + c + ") {";
	}

	std::string operator()(const Elif &elif) {
		next_indentation = indentation--;
		if (elif.cond == -1)
			return "} else {";

		std::string c = defer(elif.cond, true);
		return "} else if (" + c + ") {";
	}

	std::string operator()(const End &) {
		// TODO: verify which end?
		next_indentation = indentation--;
		return "}";
	}

	std::string operator()(const Global &global) {
		switch (global.qualifier) {
		case Global::layout_out:
			return "_lout" + std::to_string(global.binding);
		case Global::layout_in:
			return "_lin" + std::to_string(global.binding);
		default:
			break;
		}

		return "<glo:?>";
	}

	std::string operator()(const Primitive &p) {
		switch (p.type) {
		case i32:
			return std::to_string(p.idata[0]);
		case vec4:
			return "vec4("
				+ std::to_string(p.fdata[0]) + ", "
				+ std::to_string(p.fdata[1]) + ", "
				+ std::to_string(p.fdata[2]) + ", "
				+ std::to_string(p.fdata[3])
				+ ")";
		default:
			break;
		}

		return "<prim:?>";
	}

	std::string operator()(const Load &load) {
		// TODO: cache

		bool inl = inlining;
		std::string value = defer(load.src);
		if (inl)
			return value;

		_typeof_dispatcher typer(pool);
		std::string type = typer.defer(load.src);
		std::string sym = "s" + std::to_string(generator++);
		return type + " " + sym + " = " + value + ";";
	}

	std::string operator()(const Store &store) {
		std::string lhs = defer(store.dst);
		std::string rhs = defer(store.src);
		return lhs + " = " + rhs + ";";
	}

	std::string operator()(const Cmp &cmp) {
		bool inl = inlining;
		std::string sa = defer(cmp.a, inl);
		std::string sb = defer(cmp.b, inl);
		switch (cmp.type) {
		case Cmp::eq:
			return sa + " == " + sb;
		default:
			break;
		}

		return "<cmp:?>";
	}

	template <typename T>
	std::string operator()(const T &) {
		return "<?>";
	}

	std::string defer(int index, bool inl = false) {
		inlining = inl;
		if (symbols.contains(index))
			return symbols[index];

		return std::visit(*this, pool[index]);
	}

	std::string eval(const op::General &general) {
		std::string source = std::visit(*this, general) + "\n";
		std::string space(indentation << 2, ' ');
		indentation = next_indentation;
		return space + source;
	}
};

}

struct IREmitter {
	// By default the program begins at index=0
	op::General *pool;
	size_t size;
	size_t pointer;

	std::stack <int> control_flow_ends;
	std::vector <int> main;

	IREmitter() : pool(nullptr), size(0), pointer(0) {}

	void resize(size_t units) {
		op::General *old = pool;

		pool = new op::General[units];
		if (old) {
			std::memcpy(pool, old, size * sizeof(op::General));
			delete[] old;
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
		main.push_back(p);
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
			std::visit(op::_dump_dispatcher(), op);
			printf("\n");
			assert(false);
		}
	}

	// Generating GLSL source code
	std::string generate_glsl() {
		// Gather all global shader variables
		std::map <int, std::string> lins;
		std::map <int, std::string> louts;

		for (int i = 0; i < pointer; i++) {
			auto op = pool[i];
			if (!std::holds_alternative <op::Global> (op))
				continue;

			auto global = std::get <op::Global> (op);
			auto type = std::visit(op::_typeof_dispatcher(pool), op);
			if (global.qualifier == op::Global::layout_in)
				lins[global.binding] = type;
			else if (global.qualifier == op::Global::layout_out)
				louts[global.binding] = type;
		}

		op::_translate_dispatcher tdisp;
		tdisp.pool = pool;

		std::string source;

		// Version header
		source += "#version 450\n";
		source += "\n";

		// Global shader variables
		// TODO: check for vulkan target, etc
		for (const auto &[binding, type] : lins)
			source += fmt::format("layout (locatoon = {}) in {} _lin{};\n", binding, type, binding);
		source += "\n";

		for (const auto &[binding, type] : louts)
			source += fmt::format("layout (locatoon = {}) out {} _lout{};\n", binding, type, binding);
		source += "\n";

		// Main function
		source += "void main()\n";
		source += "{\n";
		for (int i : main)
			source += "    " + tdisp.eval(pool[i]);
		source += "}\n";

		return source;
	}

	// Printing the IR state
	void dump() {
		printf("GLOBALS (%4d/%4d)\n", pointer, size);
		for (size_t i = 0; i < pointer; i++) {
			if (std::ranges::find(main.begin(), main.end(), i) != main.end())
				printf("[*] ");
			else
				printf("    ");

			printf("[%4d]: ", i);
			std::visit(op::_dump_dispatcher(), pool[i]);
			printf("\n");
		}
	}

	// TODO: one per thread
	static IREmitter active;
};

IREmitter IREmitter::active;

// Interface
struct tagged {
	int loc = -1;
};

// Way to upcast C++ primitives into a workable type
template <typename T>
struct gltype {};

template <typename T>
constexpr auto type_match();

template <typename T>
concept synthesizable = requires(const T &t) {
	{ t.synthesize() } -> std::same_as <int>;
};

template <typename T, typename ... Args>
concept callbacked = requires(T &t, const Args &...args) {
	{ t.callback(args...) };
};

template <typename T, size_t binding>
struct layout_in {
	mutable int ref = -1;

	int synthesize() const {
		if (ref >= 0)
			return ref;

		auto &em = IREmitter::active;

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
};

template <typename T, size_t binding>
requires synthesizable <T>
struct layout_out : T {
	void operator=(const T &t) const {
		auto &em = IREmitter::active;

		op::TypeField type;
		type.item = type_match <T> ();
		type.next = -1;

		op::Global qualifier;
		qualifier.type = em.emit(type);
		qualifier.binding = binding;
		qualifier.qualifier = op::Global::layout_out;

		op::Store store;
		store.dst = em.emit(qualifier);
		store.src = t.synthesize();

		em.emit_main(store);
	}
};

template <typename T, typename Up, typename P, P payload>
// requires callbacked <Up>
struct phantom_type;

template <typename T, size_t N>
struct swizzle_base {};

// TODO: inherit from base 3 and add extras
template <typename T>
struct swizzle_base <T, 4> : tagged {
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
struct vec : swizzle_base <T, N> {
	T data[N];
	constexpr vec() = default;

	constexpr vec(T v) {
		for (size_t i = 0; i < N; i++)
			data[i] = v;
	}

	int synthesize() const {
		// TODO: skip if already existing cache

		auto &em = IREmitter::active;

		op::Primitive p;
		p.type = op::PrimitiveType::vec4;
		std::memcpy(p.fdata, data, N * sizeof(T));

		// TODO: only emit main if not trivial/cexpr
		return em.emit(p);
	}
};

template <typename T, size_t N, size_t M>
struct mat {};

template <>
struct gltype <int> {
	int value;
	mutable int ref = -1;
	// TODO: value or a chain of IR on its own

	gltype(int v = 0) : value(v) {}

	int synthesize() const {
		// If ref != -1...
		auto &em = IREmitter::active;

		op::Primitive p;
		p.type = op::PrimitiveType::i32;
		p.idata[0] = value;

		return (ref = em.emit(p));
	}
};

template <>
struct gltype <bool> {
	bool value;
	mutable int ref = -1;

	int synthesize() const {
		if (ref >= 0)
			return ref;

		auto &em = IREmitter::active;

		op::Primitive t;
		t.type = op::PrimitiveType::boolean;

		return (ref = em.emit(t));
	}
};

// Common types
using boolean = gltype <bool>;

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
	if constexpr (std::is_same_v <T, vec4>)
		return op::PrimitiveType::vec4;

	return op::PrimitiveType::bad;
}

template <typename T, typename U>
requires synthesizable <T> && synthesizable <U>
boolean operator==(const T &A, const U &B)
{
	auto &em = IREmitter::active;

	// TODO: ref mechanics
	int a = A.synthesize();
	int b = B.synthesize();

	op::Cmp cmp;
	cmp.a = a;
	cmp.b = b;
	cmp.type = op::Cmp::eq;

	int c = em.emit(cmp);

	return boolean { .ref = c };
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
	auto &em = IREmitter::active;
	op::Cond branch;
	branch.cond = b.synthesize();
	em.emit_main(branch);
}

void elif(boolean b)
{
	auto &em = IREmitter::active;
	op::Elif branch;
	branch.cond = b.synthesize();
	em.emit_main(branch);
}

void elif()
{
	// Treated as an else
	auto &em = IREmitter::active;
	op::Elif branch;
	branch.cond = -1;
	em.emit_main(branch);
}

void end()
{
	auto &em = IREmitter::active;
	em.emit_main(op::End());
}

// Guarantees
static_assert(synthesizable <vec4>);
static_assert(synthesizable <gltype <int>>);
static_assert(synthesizable <boolean>);
static_assert(synthesizable <layout_in <int, 0>>);

struct push_constants {};

struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;
};

void vertex_shader()
{
	layout_in <vec3, 0> position;
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

	fragment_shader();
	IREmitter::active.dump();

	auto source = IREmitter::active.generate_glsl();

	printf("\nGLSL:\n%s", source.c_str());

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(800, 800, "LearnOpenGL", NULL, NULL);
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

	GLuint program = glCreateShader(GL_FRAGMENT_SHADER);
	const char *source_c_str = source.c_str();
	glShaderSource(program, 1, &source_c_str, nullptr);
	glCompileShader(program);

	printf("program: %d\n", program);
}
