#pragma once

#include <glm/glm.hpp>

#include "../../common/meta.hpp"
#include "../array.hpp"
#include "../matrix.hpp"
#include "../vector.hpp"
#include "pad_tuple.hpp"

namespace jvl::ire {

template <builtin T>
struct solid_atomic {
	static_assert(false, "no available atomic conversion for this type");

	// For each valid conversion the following are provided:
	// - concrete host type
	// - expected alignment of the type on device
};

// 4-byte scalar elements
template <typename T>
concept native_4b = native <T> && (sizeof(T) == 4);

// Native types
template <native_4b T>
struct solid_atomic <native_t <T>> {
	using type = T;
	static constexpr size_t alignment = 4u;
};

// Vector types
template <native_4b T>
struct solid_atomic <vec <T, 2>> {
	using type = glm::tvec2 <T>;
	static constexpr size_t alignment = 8u;
};

template <native_4b T>
struct solid_atomic <vec <T, 3>> {
	using type = glm::tvec3 <T>;
	static constexpr size_t alignment = 16u;
};

template <native_4b T>
struct solid_atomic <vec <T, 4>> {
	using type = glm::tvec4 <T>;
	static constexpr size_t alignment = 16u;
};

// Matrix types
template <native_4b T>
struct solid_atomic <mat <T, 4, 4>> {
	using type = glm::tmat4x4 <T>;
	static constexpr size_t alignment = 16u;
};

// Array types
template <typename T, size_t Padding>
class padded_element {
	char _padding[Padding];
	T data;
public:
	padded_element() = default;
	padded_element(const T &value) : data(value) {}

	operator T &() {
		return data;
	}

	operator const T &() const {
		return data;
	}
};

template <builtin T, size_t N>
struct solid_atomic <static_array <T, N>> {
	using raw = solid_atomic <T> ::type;
	static constexpr size_t padding = meta::alignup(sizeof(raw), 16) - sizeof(raw);
	using element = padded_element <raw, padding>;
	using type = std::array <element, N>;
	static constexpr size_t alignment = 16u;
};

} // namespace jvl::ire