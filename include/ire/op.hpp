#pragma once

// TODO: remove
#include <iostream>

#include <fmt/printf.h>
#include <unordered_set>

#include "wrapped_types.hpp"

namespace jvl::ire::op {

enum PrimitiveType {
	bad,
	boolean, i32, f32,
	vec2, vec3, vec4,
	ivec2, ivec3, ivec4,
	mat2, mat3, mat4,
};

// Type translation
static const char *type_table[] = {
	"<BAD>",
	"bool", "int", "float",
	"vec2", "vec3", "vec4",
	"ivec2", "ivec3", "ivec4",
	"mat2", "mat3", "mat4",
};

// Atomic types
struct Global {
	int type = -1;
	int binding = -1;

	enum {
		layout_in,
		layout_out,
		push_constant,
		glsl_vertex_intrinsic_gl_Position,
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
	// TODO: decrease size since using all four is like rare (use a list instead?)
	union {
		bool b;
		float fdata[4];
		int idata[4] = {0, 0, 0, 0};
	};
};

struct Swizzle {
	enum Kind {
		x, y, z, w, xy
	} type;

	static constexpr const char *swizzle_name[] = {
		"x", "y", "z", "w", "xy"
	};

	int src = -1;
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
	// TODO: is failto actually used?
	int failto = -1;
};

struct While {
	int cond = -1;
	int failto = -1;
};

struct Elif : Cond {};

struct End {};

using General = wrapped::variant <
	Global, TypeField, Primitive, Swizzle, Cmp,
	Construct, List, Store, Load, Cond, Elif, While, End
>;

// Dispatcher types (vd = variadic dispatcher)
inline std::string type_name(const General *const pool,
		             const wrapped::hash_table <int, std::string> &struct_names,
			     int index,
			     int field)
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

// Printing instructions for debugging
void dump_ir_operation(const General &);

// Reindexing integer elements during compaction
void reindex_ir_operation(const wrapped::reindex &, General &);

// Synthesizing GLSL source code
std::string synthesize_glsl_body(const General *const,
		                 const wrapped::hash_table <int, std::string> &,
		                 const std::unordered_set <int> &,
				 size_t);

} // namespace jvl::ire::op
