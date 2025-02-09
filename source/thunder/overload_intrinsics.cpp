#include "thunder/enumerations.hpp"
#include "thunder/overload.hpp"

namespace jvl::thunder {

// Overload table for intrinsics
QualifiedType lookup_intrinsic_overload(const IntrinsicOperation &key, const std::vector <QualifiedType> &args)
{
        static const overload_table <IntrinsicOperation> table {
		// Global startus
		{ layout_local_size, {
			overload::from(none, u32),
			overload::from(none, u32, u32),
			overload::from(none, u32, u32, u32),
		} },
		
		{ layout_mesh_shader_sizes, {
			overload::from(none, u32, u32),
		} },
		
		{ emit_mesh_tasks, {
			overload::from(none, u32, u32, u32),
		} },
		
		{ set_mesh_outputs, {
			overload::from(none, u32, u32),
		} },

		// Casting operations
		{ cast_to_int, {
			overload::from(i32, i32),
			overload::from(i32, u32),
			overload::from(i32, f32),
		} },

		{ cast_to_uint, {
			overload::from(u32, i32),
			overload::from(u32, u32),
			overload::from(u32, f32),
		} },

		{ cast_to_float, {
			overload::from(f32, i32),
			overload::from(f32, u32),
			overload::from(f32, f32),
		} },

		// Trigonometric functions
                { sin, {
			overload::from(f32, f32),
			overload::from(vec2, vec2),
			overload::from(vec3, vec3),
			overload::from(vec4, vec4),
		} },
                
		{ cos, {
			overload::from(f32, f32),
			overload::from(vec2, vec2),
			overload::from(vec3, vec3),
			overload::from(vec4, vec4),
		} },
                
		{ tan, {
			overload::from(f32, f32),
			overload::from(vec2, vec2),
			overload::from(vec3, vec3),
			overload::from(vec4, vec4),
		} },

		// Inverse trigonometric functions
                { asin, { overload::from(f32, f32) } },
                { acos, { overload::from(f32, f32) } },
                { atan, {
			overload::from(f32, f32),
			overload::from(f32, f32, f32),
		} },

		// Powering functions
                { sqrt, { overload::from(f32, f32) } },
                { exp, { overload::from(f32, f32) } },
                { pow, { overload::from(f32, f32, f32) } },
                { log, { overload::from(f32, f32) } },

		// Limiting functions
		{ abs, {
			overload::from(i32, i32),
			overload::from(u32, u32),
			overload::from(f32, f32),
		} },

                { clamp, {
                        overload::from(f32, f32, f32, f32)
                } },

		{ min, {
                        overload::from(i32, i32, i32),
                        overload::from(u32, u32, u32),
                        overload::from(f32, f32, f32),

                        overload::from(vec2, vec2, vec2),
                        overload::from(vec3, vec3, vec3),
                        overload::from(vec4, vec4, vec4),
                } },

		{ max, {
                        overload::from(i32, i32, i32),
                        overload::from(u32, u32, u32),
                        overload::from(f32, f32, f32),

                        overload::from(vec2, vec2, vec2),
                        overload::from(vec3, vec3, vec3),
                        overload::from(vec4, vec4, vec4),
                } },

		{ fract, {
                        overload::from(f32, f32),
                        overload::from(vec2, vec2),
                        overload::from(vec3, vec3),
                        overload::from(vec4, vec4),
                } },

		{ floor, {
                        overload::from(f32, f32),
                        overload::from(vec2, vec2),
                        overload::from(vec3, vec3),
                        overload::from(vec4, vec4),
                } },

		{ ceil, {
                        overload::from(f32, f32),
                        overload::from(vec2, vec2),
                        overload::from(vec3, vec3),
                        overload::from(vec4, vec4),
                } },

		// Vector operations
		{ length, {
			overload::from(f32, vec2),
			overload::from(f32, vec3),
			overload::from(f32, vec4),
		} },

                { dot, {
                        overload::from(i32, ivec2, ivec2),
                        overload::from(i32, ivec3, ivec3),
                        overload::from(i32, ivec4, ivec4),

			overload::from(u32, uvec2, uvec2),
                        overload::from(u32, uvec3, uvec3),
                        overload::from(u32, uvec4, uvec4),

			overload::from(f32, vec2, vec2),
                        overload::from(f32, vec3, vec3),
                        overload::from(f32, vec4, vec4),
                } },

		{ cross, {
                        overload::from(vec3, vec3, vec3),
                } },

		{ normalize, {
                        overload::from(vec3, vec3),
                } },

		{ reflect, {
                        overload::from(vec3, vec3, vec3),
                } },

		// Miscellaneous operations
		{ mod, {
			overload::from(f32, f32, f32),
			overload::from(vec2, vec2, f32),
			overload::from(vec3, vec3, f32),
			overload::from(vec4, vec4, f32),
		} },

		{ mix, {
			overload::from(f32, f32, f32, f32),
			overload::from(vec2, vec2, vec2, f32),
			overload::from(vec3, vec3, vec3, f32),
			overload::from(vec4, vec4, vec4, f32),
		} },

		// GLSL image and sampler intrinsics
		{ glsl_texture, {
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(ivec4, 1), PlainDataType(f32)),
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(uvec4, 1), PlainDataType(f32)),
			overload::from(PlainDataType(vec4), QualifiedType::sampler(vec4, 1), PlainDataType(f32)),

			overload::from(PlainDataType(ivec4), QualifiedType::sampler(ivec4, 2), PlainDataType(vec2)),
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(uvec4, 2), PlainDataType(vec2)),
			overload::from(PlainDataType(vec4), QualifiedType::sampler(vec4, 2), PlainDataType(vec2)),

			overload::from(PlainDataType(ivec4), QualifiedType::sampler(ivec4, 3), PlainDataType(vec3)),
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(uvec4, 3), PlainDataType(vec3)),
			overload::from(PlainDataType(vec4), QualifiedType::sampler(vec4, 3), PlainDataType(vec3)),
		} },

		{ glsl_texelFetch, {
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(ivec4, 1), PlainDataType(i32), PlainDataType(i32)),
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(uvec4, 1), PlainDataType(i32), PlainDataType(i32)),
			overload::from(PlainDataType(vec4), QualifiedType::sampler(vec4, 1), PlainDataType(i32), PlainDataType(i32)),

			overload::from(PlainDataType(ivec4), QualifiedType::sampler(ivec4, 2), PlainDataType(ivec2), PlainDataType(i32)),
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(uvec4, 2), PlainDataType(ivec2), PlainDataType(i32)),
			overload::from(PlainDataType(vec4), QualifiedType::sampler(vec4, 2), PlainDataType(ivec2), PlainDataType(i32)),

			overload::from(PlainDataType(ivec4), QualifiedType::sampler(ivec4, 3), PlainDataType(ivec3), PlainDataType(i32)),
			overload::from(PlainDataType(ivec4), QualifiedType::sampler(uvec4, 3), PlainDataType(ivec3), PlainDataType(i32)),
			overload::from(PlainDataType(vec4), QualifiedType::sampler(vec4, 3), PlainDataType(ivec3), PlainDataType(i32)),
		} },

		// GLSL casting intrinsics
		{ glsl_floatBitsToInt, {
                        overload::from(i32, f32),
                        overload::from(ivec2, vec2),
                        overload::from(ivec3, vec3),
                        overload::from(ivec4, vec4),
                } },

		{ glsl_floatBitsToUint, {
                        overload::from(u32, f32),
                        overload::from(uvec2, vec2),
                        overload::from(uvec3, vec3),
                        overload::from(uvec4, vec4),
                } },

		{ glsl_intBitsToFloat, {
                        overload::from(f32, i32),
                        overload::from(vec2, ivec2),
                        overload::from(vec3, ivec3),
                        overload::from(vec4, ivec4),
                } },

		{ glsl_uintBitsToFloat, {
                        overload::from(f32, u32),
                        overload::from(vec2, uvec2),
                        overload::from(vec3, uvec3),
                        overload::from(vec4, uvec4),
                } },

		// GLSL specific intrinsics,
		{ glsl_dFdx, {
			overload::from(vec3, vec3),
		} },

		{ glsl_dFdy, {
			overload::from(vec3, vec3),
		} },

		{ glsl_dFdxFine, {
			overload::from(vec3, vec3),
		} },

		{ glsl_dFdyFine, {
			overload::from(vec3, vec3),
		} },

		{ glsl_subgroupShuffle, {
			overload::from(vec2, vec2, u32),
			overload::from(vec3, vec3, u32),
			overload::from(vec4, vec4, u32),
		} },
		
		{ glsl_barrier, { overload::from(none) } },

		{ discard, { overload::from(none) } },
        };

        return table.lookup(key, args);
}

} // namespace jvl::thunder
