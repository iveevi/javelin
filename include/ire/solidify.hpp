#pragma once

#include "aliases.hpp"
#include "../math_types.hpp"

namespace jvl::ire {

// Indexing types for tuples
template <size_t I>
struct tuple_index {};

// Programmatically easier tuples
template <typename ... Args>
struct tuple : std::tuple <Args...> {
	template <size_t I>
	auto &operator[](tuple_index <I> index) {
		return std::get <I> (*this);
	}
	
	template <size_t I>
	const auto &operator[](tuple_index <I> index) const {
		return std::get <I> (*this);
	}
};

// Holds the concrete native type for a given IRE generic type
template <typename T>
struct solid_base_t {
	struct default_invalid {};
	using type = default_invalid;
};

// Alias to skip the explicit lookup
template <typename T>
using solid_t = solid_base_t <T> ::type;

// Specializations for each acceptable type
template <>
struct solid_base_t <f32> {
	using type = float;
};

// Following the alignment specification from GLSL/OpenGL/SPIRV
struct alignas(16) aligned_float3 : float3 {};

template <>
struct solid_base_t <vec3> {
	using type = aligned_float3;
};

// Matrix types
template <>
struct solid_base_t <mat4> {
	using type = float4x4;
};

// Redirect type for uniform compatible types
template <typename ... Args>
struct solid_base_t <const_uniform_layout_t <Args...>> {
	using type = tuple <solid_t <Args>...>;
};

// Accepting uniform compatible types to translate user-defined structs
template <uniform_compatible T>
struct solid_base_t <T> {
	using layout_t = decltype(T().layout());
	using type = solid_base_t <layout_t> ::type;
};

} // namespace jvl::ire