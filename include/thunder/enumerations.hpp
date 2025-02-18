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

static_assert(__pt_end + 1 == sizeof(tbl_primitive_types) / sizeof(const char *));

//////////////////////
// Global qualifier //
//////////////////////

enum QualifierKind : int8_t {
	basic,

	parameter,

	qualifier_in,
	qualifier_out,
	qualifier_inout,

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

	// Additional qualifiers
	write_only,
	read_only,

	// Samplers
	isampler_1d,
	isampler_2d,
	isampler_3d,

	usampler_1d,
	usampler_2d,
	usampler_3d,

	sampler_1d,
	sampler_2d,
	sampler_3d,

	// Images
	iimage_1d,
	iimage_2d,
	iimage_3d,

	uimage_1d,
	uimage_2d,
	uimage_3d,

	image_1d,
	image_2d,
	image_3d,

	// Mesh shaders
	task_payload,

	// Raytracing
	ray_tracing_payload,
	ray_tracing_payload_in,
	acceleration_structure,

	// GLSL intrinsics
	glsl_FragCoord,
	glsl_FragDepth,
	glsl_InstanceID,
	glsl_InstanceIndex,
	glsl_VertexID,
	glsl_VertexIndex,
	glsl_GlobalInvocationID,
	glsl_LocalInvocationID,
	glsl_LocalInvocationIndex,
	glsl_WorkGroupID,
	glsl_WorkGroupSize,
	glsl_SubgroupInvocationID,
	glsl_Position,
	glsl_MeshVerticesEXT,
	glsl_PrimitiveTriangleIndicesEXT,
	glsl_LaunchIDEXT,
	glsl_LaunchSizeEXT,

	__gq_end
};

static constexpr const char *tbl_qualifier_kind[] = {
	"basic",

	"parameter",

	"in",
	"out",
	"inout",

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

	"write_only",
	"read_only",

	"isampler1D",
	"isampler2D",
	"isampler3D",

	"usampler1D",
	"usampler2D",
	"usampler3D",

	"sampler1D",
	"sampler2D",
	"sampler3D",

	"iimage1D",
	"iimage2D",
	"iimage3D",

	"uimage1D",
	"uimage2D",
	"uimage3D",

	"image1D",
	"image2D",
	"image3D",

	"task payload",

	"ray tracing payload",
	"ray tracing payload (in)",
	"acceleration structure",

	"glsl:FragCoord",
	"glsl:FragDepth",
	"glsl:InstanceID",
	"glsl:InstanceIndex",
	"glsl:VertexID",
	"glsl:VertexIndex",
	"glsl:GlobalInvocationID",
	"glsl:LocalInvocationID",
	"glsl:LocalInvocationIndex",
	"glsl:WorkGroupID",
	"glsl:WorkGroupSize",
	"glsl:SubgroupInvocationID",
	"glsl:Position",
	"glsl:MeshVerticesEXT",
	"glsl:PrimitiveTrianglesIndicesEXT",
	"glsl:LaunchIDEXT",
	"glsl:LaunchSizeEXT",

	"__end"
};

static_assert(__gq_end + 1 == sizeof(tbl_qualifier_kind) / sizeof(const char *));

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

static_assert(__sc_end + 1 == sizeof(tbl_swizzle_code) / sizeof(const char *));

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

// TODO: put into source file...
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

static_assert(__bk_end + 1 == sizeof(tbl_branch_kind) / sizeof(const char *));

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

static_assert(__oc_end + 1 == sizeof(tbl_operation_code) / sizeof(const char *));

/////////////////////////
// Intrinsic Operation //
/////////////////////////

enum IntrinsicOperation : uint16_t {
	discard,

	// Global status
	layout_local_size,
	layout_mesh_shader_sizes,

	emit_mesh_tasks,
	set_mesh_outputs,

	// Casting operations
	cast_to_int,	
	cast_to_ivec2,
	cast_to_ivec3,
	cast_to_ivec4,

	cast_to_uint,
	cast_to_uvec2,
	cast_to_uvec3,
	cast_to_uvec4,

	cast_to_float,
	cast_to_vec2,
	cast_to_vec3,
	cast_to_vec4,

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

	// Raytracing
	trace_ray,

	// GLSL image and sampler operations
	glsl_image_size,
	glsl_image_load,
	glsl_image_store,

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

	// GLSL subgroup operations
	glsl_subgroupShuffle,

	// GLSL synchronization operations
	glsl_barrier,

	__io_end
};

static constexpr const char *tbl_intrinsic_operation[] = {
	"discard",

	"local_size",
	"mesh_shader_size",

	"EmitMeshTasksEXT",
	"SetMeshOutputsEXT",

	"int",
	"ivec2",
	"ivec3",
	"ivec4",
	
	"uint",
	"uvec2",
	"uvec3",
	"uvec4",

	"float",
	"vec2",
	"vec3",
	"vec4",

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

	"traceRayEXT",

	"imageSize",
	"imageLoad",
	"imageStore",

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

	"subgroupShuffle",

	"barrier",

	"__end"
};

static_assert(__io_end + 1 == sizeof(tbl_intrinsic_operation) / sizeof(const char *));

///////////////////////
// Constructor modes //
///////////////////////

// TODO: static
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

static_assert(__cm_end + 1 == sizeof(tbl_constructor_mode) / sizeof(const char *));

} // namespace jvl::thunder
