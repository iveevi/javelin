#pragma once

#include "aliases.hpp"
#include "aggregate.hpp"
#include "../math_types.hpp"

namespace jvl::ire {

// Holds the concrete native type for a given IRE generic type
struct default_undefined_solid {};

template <typename T>
struct solid_base_t {
	using type = default_undefined_solid;
};

// Alias to skip the explicit lookup
template <typename T>
using solid_t = solid_base_t <T> ::type;

template <typename T>
concept solidifiable = !std::same_as <solid_t <T>, default_undefined_solid>;

// Specializations for each acceptable type
template <>
struct solid_base_t <f32> {
	using type = float;
};

template <>
struct solid_base_t <i32> {
	using type = int;
};

// Vector types
template <typename T, size_t N>
struct solid_base_t <vec <T, N>> {
	using type = vector <T, N>;
};

static_assert(std::same_as <solid_base_t <ivec2> ::type, int2>);
static_assert(std::same_as <solid_base_t <ivec3> ::type, int3>);
static_assert(std::same_as <solid_base_t <ivec4> ::type, int4>);

static_assert(std::same_as <solid_base_t <vec2> ::type, float2>);
static_assert(std::same_as <solid_base_t <vec3> ::type, float3>);
static_assert(std::same_as <solid_base_t <vec4> ::type, float4>);

// Matrix types
template <>
struct solid_base_t <mat4> {
	using type = float4x4;
};

// Generating ABI compliant structures from uniform compatible types
template <typename ... Args>
struct solid_base_t <const_uniform_layout_t <Args...>> {
	using type = aggregate <typename solid_base_t <Args> ::type...>;
};

template <uniform_compatible T>
struct solid_base_t <T> {
	using layout_t = decltype(T().layout());
	using type = solid_base_t <layout_t> ::type;
};

// User-defined structs with a value_type
template <typename T>
requires (!std::same_as <typename T::ire_value_type, default_undefined_solid>)
struct solid_base_t <T> {
	using type = T::ire_value_type;
};

} // namespace jvl::ire
