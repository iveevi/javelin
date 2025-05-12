#pragma once

#include "../array.hpp"
#include "../concepts.hpp"
#include "../matrix.hpp"
#include "../native.hpp"
#include "../vector.hpp"

namespace jvl::ire {

template <generic T>
struct is_solidifiable : std::false_type {};

// Specializing for valid types
template <native T>
struct is_solidifiable <native_t <T>> : std::true_type {};

template <native T, size_t D>
struct is_solidifiable <vec <T, D>> : std::true_type {};

template <native T, size_t N, size_t M>
struct is_solidifiable <mat <T, N, M>> : std::true_type {};

// Generalizing over aggregate types
template <bool Phantom, field_type ... Fields>
constexpr bool is_solidifiable_layout(Layout <Phantom, Fields...> *)
{
	return (is_solidifiable <typename Fields::underlying> ::value && ...);
}

template <aggregate T>
struct is_solidifiable <T> {
	using layout_t = decltype(T().layout());

	static constexpr bool value = is_solidifiable_layout((layout_t *) nullptr);
};

// Overarching concept
template <typename T>
concept solidifiable = generic <T> && is_solidifiable <T> ::value;

// More specializations building on the atomic types
template <solidifiable T, size_t N>
struct is_solidifiable <static_array <T, N>> : std::true_type {};

} // namespace jvl::ire