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
	__pt_end
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
	"__end"
};

static_assert(__pt_end + 1 == sizeof(tbl_primitive_types)/sizeof(const char *));

//////////////////////
// Global qualifier //
//////////////////////

enum GlobalQualifier : int8_t {
	parameter,
	layout_in,
	layout_out,
	push_constant,
	glsl_vertex_intrinsic_gl_Position,
	__gq_end
};

static constexpr const char *tbl_global_qualifier[] = {
	"parameter",
	"layout input",
	"layout output",
	"push_constant",
	"glsl:vertex:gl_Position",
	"__end"
};

static_assert(__gq_end + 1 == sizeof(tbl_global_qualifier)/sizeof(const char *));

//////////////////
// Swizzle Code //
//////////////////

enum SwizzleCode : int8_t {
	x, y, z, w, xy,
	__sc_end
};

static constexpr const char *tbl_swizzle_code[] = {
	"x", "y", "z", "w", "xy",
	"__end"
};

static_assert(__sc_end + 1 == sizeof(tbl_swizzle_code)/sizeof(const char *));

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

	__oc_end,
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

	"__end"
};

static_assert(__oc_end + 1 == sizeof(tbl_operation_code)/sizeof(const char *));

/////////////////////////
// Intrinsic Operation //
/////////////////////////

enum IntrinsicOperation : uint16_t {
	discard,

	sin,
	cos,
	tan,

	asin,
	acos,
	atan,
	
	sqrt,
	exp,
	pow,
	clamp,

	dot,
	cross,
	normalize,

	glsl_dFdx,
	glsl_dFdy,
	glsl_dFdxFine,
	glsl_dFdyFine,

	__io_end	
};

static constexpr const char *tbl_intrinsic_operation[] = {
	"discard",

	"sin",
	"cos",
	"tan",

	"asin",
	"acos",
	"atan",

	"sqrt",
	"exp",
	"pow",
	"clamp",

	"dot",
	"cross",
	"normalize",
	
	"dFdx",
	"dFdy",
	"dFdxFine",
	"dFdyFine",

	"__end"
};

static_assert(__io_end + 1 == sizeof(tbl_intrinsic_operation)/sizeof(const char *));

} // namespace jvl::thunder
