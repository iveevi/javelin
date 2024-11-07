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
	basic,

	parameter,
	arrays,
	
	layout_in_flat,
	layout_in_noperspective,
	layout_in_smooth,

	layout_out_flat,
	layout_out_noperspective,
	layout_out_smooth,

	push_constant,

	uniform,

	storage_buffer,
	read_only_storage_buffer,
	write_only_storage_buffer,

	shared,
	
	isampler_1d,
	isampler_2d,
	isampler_3d,
	
	usampler_1d,
	usampler_2d,
	usampler_3d,
	
	sampler_1d,
	sampler_2d,
	sampler_3d,

	glsl_intrinsic_gl_FragCoord,
	glsl_intrinsic_gl_FragDepth,
	glsl_intrinsic_gl_InstanceID,
	glsl_intrinsic_gl_InstanceIndex,
	glsl_intrinsic_gl_VertexID,
	glsl_intrinsic_gl_VertexIndex,
	glsl_intrinsic_gl_GlobalInvocationID,
	
	glsl_intrinsic_gl_Position,

	__gq_end
};

static constexpr const char *tbl_qualifier_kind[] = {
	"basic",

	"parameter",
	"arrays",

	"layout_input_flat",
	"layout_input_noperspective",
	"layout_input_smooth",

	"layout_output_flat",
	"layout_output_noperspective",
	"layout_output_smooth",

	"push_constant",

	"uniform",

	"buffer (read-write)",
	"buffer (read-only)",
	"buffer (write-only)",

	"shared",

	"isampler1D",
	"isampler2D",
	"isampler3D",

	"usampler1D",
	"usampler2D",
	"usampler3D",

	"sampler1D",
	"sampler2D",
	"sampler3D",

	"glsl:gl_FragCoord",
	"glsl:gl_FragDepth",
	"glsl:gl_InstanceID",
	"glsl:gl_InstanceIndex",
	"glsl:gl_VertexID",
	"glsl:gl_VertexIndex",
	"glsl:gl_GlobalInvocationID",
	
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
	control_flow_skip,
	control_flow_stop,
	control_flow_end,
	__bk_end
};

static constexpr const char *tbl_branch_kind[] = {
	"if",
	"else if",
	"else",
	"while",
	"for",
	"continue",
	"break",
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

	bool_not,
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

	"not",
	"or",
	"and",

	"bit_or",
	"bit_and",
	"bit_xor",
	"shift_left",
	"shift_right",

	"equals",
	"not_equals",
	"greater",
	"greater_equal",
	"less",
	"less_equal",

	"__end"
};

static_assert(__oc_end + 1 == sizeof(tbl_operation_code)/sizeof(const char *));

/////////////////////////
// Intrinsic Operation //
/////////////////////////

enum IntrinsicOperation : uint16_t {
	discard,

	// Global status
	set_local_size,

	// Casting operations
	cast_to_int,
	cast_to_uint,
	cast_to_float,

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
	abs,
	clamp,
	min,
	max,
	fract,
	floor,
	ceil,

	// Vector operations
	length,
	dot,
	cross,
	normalize,
	reflect,

	// Miscellaneous operations
	mod,
	mix,

	// GLSL image and sampler operations
	glsl_texture,
	glsl_texelFetch,

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

	"local_size",

	"int",
	"uint",
	"float",

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

	"abs",
	"clamp",
	"min",
	"max",
	"fract",
	"floor",
	"ceil",

	"length",
	"dot",
	"cross",
	"normalize",
	"reflect",

	"mod",
	"mix",

	"texture",
	"texelFetch",

	"dFdx",
	"dFdy",
	"dFdxFine",
	"dFdyFine",

	"floatBitsToInt",
	"floatBitsToUint",
	"intBitsToFloat",
	"uintBitsToFloat",

	"__end"
};

static_assert(__io_end + 1 == sizeof(tbl_intrinsic_operation)/sizeof(const char *));

///////////////////////
// Constructor modes //
///////////////////////

enum ConstructorMode : uint8_t {
	normal,
	assignment,
	transient,
	__cm_end,
};

static constexpr const char *tbl_constructor_mode[] = {
	"normal",
	"assignment",
	"transient",
	"__end",
};

} // namespace jvl::thunder
