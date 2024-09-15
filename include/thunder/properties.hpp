#pragma once

#include "enumerations.hpp"

namespace jvl::thunder {

// Enumeration properties
constexpr bool vector_type(PrimitiveType primitive)
{
	switch (primitive) {
	case ivec2:
	case ivec3:
	case ivec4:
	case uvec2:
	case uvec3:
	case uvec4:
	case vec2:
	case vec3:
	case vec4:
		return true;
	default:
		return false;
	}
}

constexpr size_t vector_component_count(PrimitiveType primitive)
{
	switch (primitive) {
	case ivec2:
	case uvec2:
	case vec2:
		return 2;
	case ivec3:
	case uvec3:
	case vec3:
		return 3;
	case ivec4:
	case uvec4:
	case vec4:
		return 4;
	default:
		return 0;
	}
}

constexpr PrimitiveType swizzle_type_of(PrimitiveType primitive, SwizzleCode code)
{
	// TODO: might have to handle multiswizzle
	switch (primitive) {
	case ivec2:
	case ivec3:
	case ivec4:
		return i32;
	case uvec2:
	case uvec3:
	case uvec4:
		return u32;
	case vec2:
	case vec3:
	case vec4:
		return f32;
	default:
		return bad;
	}
}

constexpr bool sampler_kind(QualifierKind kind)
{
	switch (kind) {
	case isampler_1d:
	case isampler_2d:
	case sampler_1d:
	case sampler_2d:
		return true;
	default:
		return false;
	}
}

constexpr PrimitiveType sampler_result(QualifierKind kind)
{
	switch (kind) {
	case isampler_1d:
	case isampler_2d:
		return ivec4;
	case sampler_1d:
	case sampler_2d:
		return vec4;
	default:
		return bad;
	}
}

constexpr int32_t sampler_dimension(QualifierKind kind)
{
	switch (kind) {
	case isampler_1d:
	case sampler_1d:
		return 1;
	case isampler_2d:
	case sampler_2d:
		return 2;
	default:
		return -1;
	}
}

} // namespace jvl::thunder