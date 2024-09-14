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

enum QualifierKind : int8_t {
	parameter,
	arrays,
	
	layout_in_flat,
	layout_in_noperspective,
	layout_in_smooth,

	layout_out_flat,
	layout_out_noperspective,
	layout_out_smooth,

	push_constant,

	glsl_intrinsic_gl_FragCoord,
	glsl_intrinsic_gl_FragDepth,
	glsl_intrinsic_gl_VertexID,
	glsl_intrinsic_gl_VertexIndex,
	
	glsl_intrinsic_gl_Position,

	__gq_end
};

static constexpr const char *tbl_qualifier_kind[] = {
	"parameter",
	"arrays",

	"layout_input_flat",
	"layout_input_noperspective",
	"layout_input_smooth",

	"layout_output_flat",
	"layout_output_noperspective",
	"layout_output_smooth",

	"push_constant",

	"glsl:gl_FragCoord",
	"glsl:gl_FragDepth",
	"glsl:gl_VertexID",
	"glsl:gl_VertexIndex",
	
	"glsl:gl_Position",

	"__end"
};

static_assert(__gq_end + 1 == sizeof(tbl_qualifier_kind)/sizeof(const char *));

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

	modulus,

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

	"modulus",

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
	log,

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
	"log",

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
