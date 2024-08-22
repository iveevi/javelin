#pragma once

#include <cstdint>

namespace jvl::thunder {

/////////////////////
// Primitive types //
/////////////////////

enum PrimitiveType : int8_t {
	bad,
	none,
	boolean,
	i32,
	f32,
	vec2,
	vec3,
	vec4,
	ivec2,
	ivec3,
	ivec4,
	mat2,
	mat3,
	mat4,
};

static const char *tbl_primitive_types[] = {
	"<BAD>",
	"void",
	"bool",
	"int",
	"float",
	"vec2",
	"vec3",
	"vec4",
	"ivec2",
	"ivec3",
	"ivec4",
	"mat2",
	"mat3",
	"mat4",
};

//////////////////////
// Global qualifier //
//////////////////////

enum GlobalQualifier : int8_t {
	parameter,
	layout_in,
	layout_out,
	push_constant,
	glsl_vertex_intrinsic_gl_Position,
};

static constexpr const char *tbl_global_qualifier[] = {
	"parameter",
	"layout input",
	"layout output",
	"push_constant",
	"glsl:vertex:gl_Position"
};

//////////////////
// Swizzle Code //
//////////////////

enum SwizzleCode : int8_t {
	x, y, z, w, xy
};

static constexpr const char *tbl_swizzle_code[] = {
	"x", "y", "z", "w", "xy"
};

////////////////////
// Operation Code //
////////////////////

enum OperationCode : uint8_t {
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
};
	
static constexpr const char *tbl_operation_code[] = {
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

} // namespace jvl::thunder
