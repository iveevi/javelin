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

constexpr bool intrinsic_kind(QualifierKind kind)
{
	switch (kind) {
	case acceleration_structure:
		return true;
	default:
		break;
	}

	return false;
}

constexpr bool image_kind(QualifierKind kind)
{
	switch (kind) {
	case iimage_1d:
	case iimage_2d:
	case iimage_3d:
	case uimage_1d:
	case uimage_2d:
	case uimage_3d:
	case image_1d:
	case image_2d:
	case image_3d:
		return true;
	default:
		return false;
	}
}
constexpr PrimitiveType image_result(QualifierKind kind)
{
	switch (kind) {
	case iimage_1d:
	case iimage_2d:
	case iimage_3d:
		return ivec4;
	case uimage_1d:
	case uimage_2d:
	case uimage_3d:
		return uvec4;
	case image_1d:
	case image_2d:
	case image_3d:
		return vec4;
	default:
		return bad;
	}
}

constexpr int32_t image_dimension(QualifierKind kind)
{
	switch (kind) {
	case iimage_1d:
	case uimage_1d:
	case image_1d:
		return 1;
	case iimage_2d:
	case uimage_2d:
	case image_2d:
		return 2;
	case iimage_3d:
	case uimage_3d:
	case image_3d:
		return 3;
	default:
		return -1;
	}
}

constexpr bool sampler_kind(QualifierKind kind)
{
	switch (kind) {
	case isampler_1d:
	case isampler_2d:
	case isampler_3d:
	case usampler_1d:
	case usampler_2d:
	case usampler_3d:
	case sampler_1d:
	case sampler_2d:
	case sampler_3d:
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
	case isampler_3d:
		return ivec4;
	case usampler_1d:
	case usampler_2d:
	case usampler_3d:
		return uvec4;
	case sampler_1d:
	case sampler_2d:
	case sampler_3d:
		return vec4;
	default:
		return bad;
	}
}

constexpr int32_t sampler_dimension(QualifierKind kind)
{
	switch (kind) {
	case isampler_1d:
	case usampler_1d:
	case sampler_1d:
		return 1;
	case isampler_2d:
	case usampler_2d:
	case sampler_2d:
		return 2;
	case isampler_3d:
	case usampler_3d:
	case sampler_3d:
		return 3;
	default:
		return -1;
	}
}

} // namespace jvl::thunder