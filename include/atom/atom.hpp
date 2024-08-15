#pragma once

#include <fmt/printf.h>
#include <unordered_set>

#include "../wrapped_types.hpp"

namespace jvl::atom {

using index_t = int16_t;

enum PrimitiveType : int8_t {
	bad, none,
	boolean, i32, f32,
	vec2, vec3, vec4,
	ivec2, ivec3, ivec4,
	mat2, mat3, mat4,
};

// Type translation
static const char *type_table[] = {
	"<BAD>", "void",
	"bool", "int", "float",
	"vec2", "vec3", "vec4",
	"ivec2", "ivec3", "ivec4",
	"mat2", "mat3", "mat4",
};

// TODO: pack everything, manually pad to align

// Atomic types
struct Global {
	index_t type = -1;
	index_t binding = -1;

	enum : int8_t {
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
	index_t down = -1;

	// Next field
	index_t next = -1;
};

struct Primitive {
	union {
		bool b;
		float fdata;
		int idata;
	};

	PrimitiveType type = bad;
};

struct Swizzle {
	enum Kind : int8_t {
		x, y, z, w, xy
	} type;

	index_t src = -1;

	static constexpr const char *name[] = {
		"x", "y", "z", "w", "xy"
	};
};

struct Operation {
enum : uint8_t {
		unary_negation,

		addition,
		subtraction,
		multiplication,
		division,

		equals,
		not_equals,
		cmp_ge,
		cmp_geq,
		cmp_le,
		cmp_leq,
	} type;

	index_t args = -1;
	index_t ret = -1;

	static constexpr const char *name[] = {
		"negation",

		"addition",
		"subtraction",
		"multiplication",
		"division",

		"equals",
		"not_equals",
		"cmp_ge",
		"cmp_geq",
		"cmp_le",
		"cmp_leq",
	};
};

#pragma pack(push, 1)
struct Intrinsic {
	// TODO: index in a table of intrinsics, then cache intrinsics?
	const char *name = nullptr;
	index_t args = -1;
	index_t ret = -1;
};
#pragma pack(pop)

struct List {
	index_t item = -1;
	index_t next = -1;
};

struct Construct {
	index_t type = -1;
	index_t args = -1;
};

struct Store {
	index_t dst = -1;
	index_t src = -1;
};

struct Load {
	index_t src = -1;
	index_t idx = -1; // Arrays or structures
};

struct Cond {
	index_t cond = -1;
	// TODO: is failto actually used?
	index_t failto = -1;
};

struct While {
	index_t cond = -1;
	index_t failto = -1;
};

struct Elif : Cond {};

struct Returns {
	index_t type = -1;
	index_t args = -1;
};

struct End {};

using General = wrapped::variant <
	Global, TypeField, Primitive,
	Swizzle, Operation, Construct,
	Intrinsic, List, Store, Load,
	Cond, Elif, While, Returns, End
>;

// TODO: move to guarantees.hpp
static_assert(sizeof(Global)    == 6);
static_assert(sizeof(TypeField) == 6);
static_assert(sizeof(Primitive) == 8);
static_assert(sizeof(Swizzle)   == 4);
static_assert(sizeof(Operation) == 6);
static_assert(sizeof(Intrinsic) == 12);
static_assert(sizeof(List)      == 4);
static_assert(sizeof(Construct) == 4);
static_assert(sizeof(Store)     == 4);
static_assert(sizeof(Load)      == 4);
static_assert(sizeof(Cond)      == 4);
static_assert(sizeof(While)     == 4);
static_assert(sizeof(Elif)      == 4);
static_assert(sizeof(Returns)   == 4);
static_assert(sizeof(End)       == 1);
static_assert(sizeof(General)   == 16);

// Dispatcher types (vd = variadic dispatcher)
std::string type_name(const General *const,
		      const wrapped::hash_table <int, std::string> &,
		      int, int);

// Printing instructions for debugging
void dump_ir_operation(const General &);

// Reindexing integer elements during compaction
void reindex_ir_operation(const wrapped::reindex <index_t> &, General &);

// Synthesizing GLSL source code
std::string synthesize_glsl_body(const General *const,
		                 const wrapped::hash_table <int, std::string> &,
		                 const std::unordered_set <atom::index_t> &,
				 size_t);

} // namespace jvl::atom
