#pragma once

#include <cstdint>
#include <cstdlib>

namespace jvl::thunder {

/////////////////////
// Primitive types //
/////////////////////

enum PrimitiveType : int16_t {
	bad,
	
	// end of structure
	nil,

	// void type
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

	// Higher precision types
	u64,

	__pt_end
};

extern const char *tbl_primitive_types[__pt_end];

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

	uniform_buffer,

	storage_buffer,

	buffer_reference,

	shared,

	// Memory qualifiers
	writeonly,
	readonly,
	scalar,

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
	hit_attribute,
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
	glsl_InstanceCustomIndexEXT,
	glsl_PrimitiveID,
	glsl_ObjectToWorldEXT,
	glsl_WorldRayDirectionEXT,

	__gq_end
};

extern const char *tbl_qualifier_kind[__gq_end];

//////////////////
// Swizzle Code //
//////////////////

enum SwizzleCode : int8_t {
	x, y, z, w, xy,
	__sc_end
};

extern const char *tbl_swizzle_code[__sc_end];

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

extern const char *tbl_branch_kind[__bk_end];

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

extern const char *tbl_operation_code[__oc_end];

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

	cast_to_uint64,

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

extern const char *tbl_intrinsic_operation[__io_end];

///////////////////////
// Constructor modes //
///////////////////////

enum ConstructorMode : uint8_t {
	normal,
	assignment,
	transient,
	__cm_end,
};

extern const char *tbl_constructor_mode[__cm_end];

} // namespace jvl::thunder
