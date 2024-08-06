#pragma once

// TODO: remove
#include <iostream>

#include <fmt/printf.h>

#include "wrapped_types.hpp"

namespace jvl::ire::op {

enum PrimitiveType {
	bad,
	boolean,
	i32,
	f32,
	vec2,
	vec3,
	vec4,
	mat4,
};

// Atomic types
struct Global {
	int type = -1;
	int binding = -1;

	enum {
		layout_in,
		layout_out,
		push_constant
	} qualifier;
};

struct TypeField {
	// If the current field is a
	// primitive, then item != BAD
	PrimitiveType item = bad;

	// Otherwise it is a nested struct
	int down = -1;

	// Next field
	int next = -1;
};

struct Primitive {
	PrimitiveType type = bad;
	union {
		bool b;
		float fdata[4];
		int idata[4] = {0, 0, 0, 0};
	};
};

struct Swizzle {
	enum Kind { x, y, z, w, xy } type;

	constexpr Swizzle(Kind t) : type(t) {}
};

struct Cmp {
	int a = -1;
	int b = -1;
	enum {
		eq,
		neq,
	} type;
};

struct List {
	int item = -1;
	int next = -1;
};

struct Construct {
	int type = -1;
	int args = -1;
};

struct Store {
	int dst = -1;
	int src = -1;
};

struct Load {
	int src = -1;
	int idx = -1; // Arrays or structures
};

struct Cond {
	int cond = -1;
	int failto = -1;
};

struct Elif : Cond {};

struct End {};

using General = wrapped::variant <
	Global, TypeField, Primitive, Swizzle, Cmp,
	Construct, List, Store, Load, Cond, Elif, End
>;

// Type translation
static const char *type_table[] = {
	"<BAD>", "bool", "int", "float", "vec2", "vec3", "vec4", "mat4",
};

// Dispatcher types (vd = variadic dispatcher)
inline std::string type_name(const General *const pool, const wrapped::hash_table <int, std::string> &struct_names, int index, int field)
{
	if (struct_names.contains(index)) {
		if (field == -1)
			return struct_names.at(index);

		index -= std::max(0, field);
		field = 0;
	}

	General g = pool[index];
	if (auto global = g.get <Global> ()) {
		return type_name(pool, struct_names, global->type, field);
	} else if (auto tf = g.get <TypeField> ()) {
		if (tf->item != bad)
			return type_table[tf->item];
	}

	return "<BAD>";
}

struct dump_vd {
	void operator()(const Global &);
	void operator()(const TypeField &);
	void operator()(const Primitive &);
	void operator()(const List &);
	void operator()(const Construct &);
	void operator()(const Store &);
	void operator()(const Load &);
	void operator()(const Cmp &);
	void operator()(const Cond &);
	void operator()(const Elif &);
	void operator()(const End &);

	template <typename T>
	void operator()(const T &)
	{
		printf("<?>");
	}
};

struct translate_glsl_vd {
	op::General *pool;

	int generator = 0;
	int indentation = 0;
	int next_indentation = 0;
	bool inlining = false;

	wrapped::hash_table<int, std::string> symbols;
	wrapped::hash_table<int, std::string> struct_names;

	std::string operator()(const Cond &);
	std::string operator()(const Elif &);
	std::string operator()(const End &);
	std::string operator()(const Global &);
	std::string operator()(const Primitive &);
	std::string operator()(const Load &);
	std::string operator()(const Store &);
	std::string operator()(const Cmp &);
	std::string operator()(const Construct &);

	template <typename T>
	std::string operator()(const T &)
	{
		return "<?>";
	}

	std::string defer(int, bool = false);
	std::string eval(int);
};

struct reindex_vd {
	wrapped::hash_table<int, int> &reindex;

	void operator()(Primitive &);
	void operator()(Swizzle &);
	void operator()(End &);
	void operator()(Global &);
	void operator()(TypeField &);
	void operator()(Cmp &);
	void operator()(List &);
	void operator()(Construct &);
	void operator()(Store &);
	void operator()(Load &);
	void operator()(Cond &);
	void operator()(Elif &);
};

} // namespace jvl::ire::op
