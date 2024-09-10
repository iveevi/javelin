#pragma once

#include <cstdint>
#include <cstdlib>

namespace jvl::thunder {

/////////////////////
// Primitive types //
/////////////////////

enum PrimitiveType : int8_t {
	bad,
	none,

	// Scalar types
	boolean,
	i32,
	u32,
	f32,

	// Vector types
	vec2,
	vec3,
	vec4,

	ivec2,
	ivec3,
	ivec4,

	uvec2,
	uvec3,
	uvec4,

	// Matrix types
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
	"uint",
	"float",

	"vec2",
	"vec3",
	"vec4",

	"ivec2",
	"ivec3",
	"ivec4",

	"uvec2",
	"uvec3",
	"uvec4",

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

/////////////////////
// Branching Kinds //
/////////////////////

enum BranchKind : int8_t {
	conditional_if,
	conditional_else_if,
	conditional_else,
	loop_while,
	loop_for,
	control_flow_end,
	__bk_end
};

static constexpr const char *tbl_branch_kind[] = {
	"if",
	"else if",
	"else",
	"while",
	"for",
	"end",
	"__end"
};

static_assert(__bk_end + 1 == sizeof(tbl_branch_kind)/sizeof(const char *));

////////////////////
// Operation Code //
////////////////////

enum OperationCode : uint8_t {
	unary_negation,

	addition,
	subtraction,
	multiplication,
	division,

	bool_or,
	bool_and,

	bit_or,
	bit_and,
	bit_xor,
	bit_shift_left,
	bit_shift_right,

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

	"or",
	"and",

	"bit or",
	"bit and",
	"bit xor",
	"shift left",
	"shift right",

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

	// Trigonometric functions
	sin,
	cos,
	tan,

	// Inverse trigonometric functions
	asin,
	acos,
	atan,

	// Powering functions
	sqrt,
	exp,
	pow,

	// Limiting functions
	clamp,
	min,
	max,
	fract,
	floor,
	ceil,

	// Vector operations
	dot,
	cross,
	normalize,
	reflect,

	// GLSL image buffer operations
	glsl_dFdx,
	glsl_dFdy,
	glsl_dFdxFine,
	glsl_dFdyFine,

	// GLSL casting operations
	glsl_floatBitsToInt,
	glsl_floatBitsToUint,
	glsl_intBitsToFloat,
	glsl_uintBitsToFloat,

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
	"min",
	"max",
	"fract",
	"floor",
	"ceil",

	"dot",
	"cross",
	"normalize",
	"reflect",

	"dFdx",
	"dFdy",
	"dFdxFine",
	"dFdyFine",

	"glsl_floatBitsToInt",
	"glsl_floatBitsToUint",
	"glsl_intBitsToFloat",
	"glsl_uintBitsToFloat",

	"__end"
};

static_assert(__io_end + 1 == sizeof(tbl_intrinsic_operation)/sizeof(const char *));

} // namespace jvl::thunder
