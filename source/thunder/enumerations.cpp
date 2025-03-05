#include "thunder/enumerations.hpp"

namespace jvl::thunder {

/////////////////////
// Primitive types //
/////////////////////

const char *tbl_primitive_types[] = {
	"none",
	
	"nil",
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
	"mat4x3",
	"mat3x4",

	"uint64_t",
};

//////////////////////
// Global qualifier //
//////////////////////

const char *tbl_qualifier_kind[] = {
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

	"storage buffer",

	"buffer reference",

	"shared",

	"writeonly",
	"readonly",
	"scalar",

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

	"hit attribute",
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
	"glsl:InstanceCustomIndexEXT",
	"glsl:PrimitiveID",
	"glsl:ObjectToWorldEXT",
	"glsl:WorldRayDirectionEXT",
};

//////////////////
// Swizzle Code //
//////////////////

const char *tbl_swizzle_code[] = {
	"x", "y", "z", "w", "xy",
};

/////////////////////
// Branching Kinds //
/////////////////////

const char *tbl_branch_kind[] = {
	"if",
	"else if",
	"else",
	"while",
	"for",
	"continue",
	"break",
	"end",
};

////////////////////
// Operation Code //
////////////////////

const char *tbl_operation_code[] = {
	"negate",

	"add",
	"subtract",
	"multiply",
	"divide",

	"mod",

	"lnot",
	"lor",
	"land",

	"bor",
	"band",
	"bxor",
	"shle",
	"shri",

	"eq",
	"neq",
	"ge",
	"geq",
	"le",
	"leq",

	"swz.x",
	"swz.y",
	"swz.z",
	"swz.w",
};

/////////////////////////
// Intrinsic Operation //
/////////////////////////

const char *tbl_intrinsic_operation[] = {
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

	"uint64_t",

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
};

///////////////////////
// Constructor modes //
///////////////////////

const char *tbl_constructor_mode[] = {
	"normal",
	"assignment",
	"global",
};

} // namespace jvl::thunder