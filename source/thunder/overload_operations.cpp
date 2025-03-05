#include "thunder/overload.hpp"

namespace jvl::thunder {

// Overload table for operations
QualifiedType lookup_operation_overload(const OperationCode &key, const std::vector <QualifiedType> &args)
{
	static const overload_list arithmetic_overloads {
		overload::from(i32, i32, i32),
		overload::from(u32, u32, u32),
		overload::from(f32, f32, f32),

		overload::from(f32, f32, i32),
		overload::from(f32, i32, f32),

		overload::from(ivec2, ivec2, ivec2),
		overload::from(ivec3, ivec3, ivec3),
		overload::from(ivec4, ivec4, ivec4),

		overload::from(uvec2, uvec2, uvec2),
		overload::from(uvec3, uvec3, uvec3),
		overload::from(uvec4, uvec4, uvec4),

		overload::from(vec2, vec2, vec2),
		overload::from(vec3, vec3, vec3),
		overload::from(vec4, vec4, vec4),

		overload::from(ivec3, ivec3, i32),
		overload::from(ivec3, i32, ivec3),

		overload::from(uvec2, uvec2, u32),
		overload::from(uvec2, u32, uvec2),

		overload::from(uvec3, uvec3, u32),
		overload::from(uvec3, u32, uvec3),

		overload::from(vec4, vec4, f32),
		overload::from(vec4, f32, vec4),
		overload::from(vec3, vec3, f32),
		overload::from(vec3, f32, vec3),
		overload::from(vec2, vec2, f32),
		overload::from(vec2, f32, vec2),
	};

	static const overload_list matrix_multiplication_overloads {
		overload::from(vec2, mat2, vec2),
		overload::from(vec3, mat3, vec3),
		overload::from(vec4, mat4, vec4),
		
		overload::from(vec3, mat4x3, vec4),
		overload::from(vec4, vec3, mat4x3),
	};

	static const overload_list bitwise_operator_overloads {
		overload::from(i32, i32, i32),
		overload::from(ivec2, ivec2, ivec2),
		overload::from(ivec3, ivec3, ivec3),
		overload::from(ivec4, ivec4, ivec4),

		overload::from(u32, u32, u32),
		overload::from(uvec2, uvec2, uvec2),
		overload::from(uvec2, uvec2, u32),
		overload::from(uvec3, uvec3, uvec3),
		overload::from(uvec3, uvec3, u32),
		overload::from(uvec4, uvec4, uvec4),
		overload::from(uvec4, uvec4, u32),
		
		overload::from(u64, u64, u64),
	};

	static const overload_list comparison_overloads {
		overload::from(boolean, i32, i32),
		overload::from(boolean, f32, f32),
		overload::from(boolean, u32, u32),
	};

	static const overload_list shift_overloads {
		overload::from(i32, i32, i32),
		overload::from(i32, i32, u32),

		overload::from(u32, u32, u32),
		overload::from(u32, u32, i32),
		
		overload::from(u64, u64, u32),
		overload::from(u64, u64, i32),

		overload::from(uvec3, uvec3, u32),
		overload::from(uvec3, uvec3, i32),
	};

        static const overload_table <OperationCode> table {
		{ unary_negation, {
			overload::from(i32, i32),
			overload::from(ivec2, ivec2),
			overload::from(ivec3, ivec3),
			overload::from(ivec4, ivec4),
			
			overload::from(f32, f32),
			overload::from(vec2, vec2),
			overload::from(vec3, vec3),
			overload::from(vec4, vec4),
		} },

		{ addition, arithmetic_overloads },
		{ subtraction, arithmetic_overloads },
		{ multiplication, concat(arithmetic_overloads, matrix_multiplication_overloads) },
		{ division, arithmetic_overloads },

		{ modulus, {
			overload::from(i32, i32, i32),
			overload::from(u32, u32, u32),

			overload::from(ivec2, ivec2, i32),
			overload::from(ivec3, ivec3, i32),
			overload::from(ivec4, ivec4, i32),

			overload::from(uvec2, uvec2, u32),
			overload::from(uvec3, uvec3, u32),
			overload::from(uvec4, uvec4, u32),
		} },

		{ bool_not, { overload::from(boolean, boolean) } },
		{ bool_or, { overload::from(boolean, boolean, boolean) } },
		{ bool_and, { overload::from(boolean, boolean, boolean) } },

		{ bit_and, bitwise_operator_overloads },
		{ bit_or, bitwise_operator_overloads },
		{ bit_xor, bitwise_operator_overloads },

		{ bit_shift_left, shift_overloads },
		{ bit_shift_right, shift_overloads },

		{ cmp_ge, comparison_overloads },
		{ cmp_geq, comparison_overloads },
		{ cmp_le, comparison_overloads },
		{ cmp_leq, comparison_overloads },
		{ equals, comparison_overloads },
		{ not_equals, comparison_overloads },
	};

	return table.lookup(key, args);
}

} // namespace jvl::thunder
