#pragma once

#include <glm/glm.hpp>

#include "aggregate.hpp"
#include "aliases.hpp"
#include "uniform_layout.hpp"

namespace jvl::ire {

// Forcing fields to be aligned
struct alignas(16) aligned_vec3 : glm::vec3 {
	using glm::vec3::vec3;

	aligned_vec3(const glm::vec3 &v) : glm::vec3(v) {}
};

struct alignas(16) aligned_uvec3 : glm::uvec3 {
	using glm::uvec3::uvec3;

	aligned_uvec3(const glm::uvec3 &v) : glm::uvec3(v) {}
};

struct alignas(16) aligned_ivec3 : glm::ivec3 {
	using glm::ivec3::ivec3;

	aligned_ivec3(const glm::ivec3 &v) : glm::ivec3(v) {}
};

template <size_t A>
struct aligned_bool : std::array <bool, A / sizeof(bool)> {
	aligned_bool(bool x = 0) {
		this->operator[](0) = x;
	}

	operator bool() const {
		return this->operator[](0);
	}
};

template <size_t A>
struct aligned_int32_t : std::array <int32_t, A / sizeof(int32_t)> {
	aligned_int32_t(int32_t x = 0) {
		this->operator[](0) = x;
	}

	operator int32_t() const {
		return this->operator[](0);
	}
};

template <size_t A>
struct aligned_uint32_t : std::array <uint32_t, A / sizeof(uint32_t)> {
	aligned_uint32_t(uint32_t x = 0) {
		this->operator[](0) = x;
	}

	operator uint32_t() const {
		return this->operator[](0);
	}
};

template <size_t A>
struct aligned_float : std::array <uint32_t, A / sizeof(float)> {
	aligned_float(uint32_t x = 0) {
		this->operator[](0) = x;
	}

	operator float() const {
		return this->operator[](0);
	}
};

// Concatenating aggregates
template <typename T, typename A>
struct aggregate_insert {};

template <typename T, typename ... Args>
struct aggregate_insert <T, aggregate_storage <Args...>> {
	using type = aggregate_storage <T, Args...>;
};

// Building aggregates recursively
template <size_t Offset, typename ... Args>
struct solid_builder {};

template <size_t Offset>
struct solid_builder <Offset> {
	static constexpr size_t pad = 0;
	using type = aggregate_storage <>;
};

template <typename T>
struct error_solid {
	static_assert(false, "Solidifying the current type is currently unsupported");
};

template <size_t Offset, generic T, typename ... Args>
struct solid_builder <Offset, T, Args...> {
	static constexpr auto error = error_solid <T> ();
	using type = aggregate_storage <>;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, boolean, Args...> {
	static constexpr size_t pad = 0;
	using rest = solid_builder <Offset + 4, Args...>;
	using elem = std::conditional_t <rest::pad == 0, bool, aligned_bool <16>>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, i32, Args...> {
	static constexpr size_t pad = 0;
	using rest = solid_builder <Offset + 4, Args...>;
	using elem = std::conditional_t <rest::pad == 0, int32_t, aligned_int32_t <16>>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, u32, Args...> {
	static constexpr size_t pad = 0;
	using rest = solid_builder <Offset + 4, Args...>;
	using elem = std::conditional_t <rest::pad == 0, uint32_t, aligned_uint32_t <16>>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, f32, Args...> {
	static constexpr size_t pad = 0;
	using rest = solid_builder <Offset + 4, Args...>;
	using elem = std::conditional_t <rest::pad == 0, float, aligned_float <16>>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, vec3, Args...> {
	static constexpr size_t rounded = ((Offset + 16 - 1) / 16) * 16;
	static constexpr size_t pad = (Offset % 16 == 0) ? 0 : (rounded - Offset);
	using rest = solid_builder <rounded + 12, Args...>;
	using elem = std::conditional_t <rest::pad == 0, glm::vec3, aligned_vec3>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, ivec3, Args...> {
	static constexpr size_t rounded = ((Offset + 16 - 1) / 16) * 16;
	static constexpr size_t pad = (Offset % 16 == 0) ? 0 : (rounded - Offset);
	using rest = solid_builder <rounded + 12, Args...>;
	using elem = std::conditional_t <rest::pad == 0, glm::ivec3, aligned_ivec3>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, uvec3, Args...> {
	static constexpr size_t rounded = ((Offset + 16 - 1) / 16) * 16;
	static constexpr size_t pad = (Offset % 16 == 0) ? 0 : (rounded - Offset);
	using rest = solid_builder <rounded + 12, Args...>;
	using elem = std::conditional_t <rest::pad == 0, glm::uvec3, aligned_uvec3>;
	using type = aggregate_insert <elem, typename rest::type> ::type;
};

template <size_t Offset, typename ... Args>
struct solid_builder <Offset, mat4, Args...> {
	static constexpr size_t rounded = ((Offset +  64 - 1) / 64) * 64;
	static constexpr size_t pad = (Offset % 16 == 0) ? 0 : (rounded - Offset);
	using rest = solid_builder <rounded + 64, Args...>;
	using type = aggregate_insert <glm::mat4, typename rest::type> ::type;
};

template <size_t Offset, string_literal_group group, typename ... Args>
struct solid_builder <Offset, higher_const_uniform_layout_t <group, Args...>> {
	using type = solid_builder <Offset, Args...> ::type;
};

template <size_t Offset, aggregate T>
struct solid_builder <Offset, T> {
	using layout_t = decltype(T().layout());
	using type = solid_builder <Offset, layout_t> ::type;
};

// Additional simplification
template <typename T>
struct simplify {
	using type = T;
};

template <typename T>
struct simplify <aggregate_storage <T>> {
	using type = T;
};

// Alias
template <generic T>
using solid_t = simplify <typename solid_builder <0, T> ::type> ::type;

template <generic T>
static constexpr size_t solid_size = sizeof(solid_t <T>);

} // namespace jvl::ire