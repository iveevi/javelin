#pragma once

#include "../../common/meta.hpp"
#include "../concepts.hpp"
#include "pad_tuple.hpp"
#include "atomic.hpp"

namespace jvl::ire {

// Iterative scaffolding
// TODO: bool for scalar
// TODO: nested aggregates...
template <size_t Offset, size_t Index, size_t Alignment, generic ... Ts>
struct solid_padded {
	static_assert(false, "failed to solidify structure...");
};

template <size_t Offset, size_t Index, size_t Alignment>
struct solid_padded <Offset, Index, Alignment> {
	using result = meta::type_packet <>;
};

// Handling final alignment with a terminal marker
struct solid_padded_terminal {
	// Only to satisfy the builtin constraint
	auto synthesize() const -> cache_index_t;
};

template <size_t Offset, size_t Index, size_t Alignment>
struct solid_padded <Offset, Index, Alignment, solid_padded_terminal> {
	static constexpr size_t rounded = meta::alignup(Offset, Alignment);
	static constexpr size_t pad = (Offset % Alignment == 0) ? 0 : (rounded - Offset);

	using element = padded_end <pad>;
	using result = meta::type_packet <element>;
};

// Supported built in types have an atomic conversion
template <size_t Offset, size_t Index, size_t Alignment, builtin T, generic ... Ts>
struct solid_padded <Offset, Index, Alignment, T, Ts...> {
	using conversion = solid_atomic <T>;

	using data = conversion::type;
	static constexpr size_t alignment = conversion::alignment;

	static constexpr size_t rounded = meta::alignup(Offset, alignment);
	static constexpr size_t pad = (Offset % alignment == 0) ? 0 : (rounded - Offset);

	using next = solid_padded <rounded + sizeof(data), Index + 1, std::max(Alignment, alignment), Ts...>;
	using element = padded <data, pad, Index>;
	using result = next::result::template push_front <element>;
};

// Conversion from type_packet to pad_tuple
template <typename ... Args>
constexpr auto pad_tuple_conversion_proxy(const meta::type_packet <Args...> &)
{
	return pad_tuple <Args...> ();
}

template <typename Packet>
using pad_tuple_conversion_t = decltype(pad_tuple_conversion_proxy(Packet()));

// Tying everything together
template <typename ... Args>
struct solid_padded_builder {};

template <builtin T>
struct solid_padded_builder <T> {
	using type = solid_atomic <T> ::type;
};

template <bool Phantom, field_type ... Fields>
struct solid_padded_builder <Layout <Phantom, Fields...>> {
	// TODO: scan fields to ensure solidifiable property...
	using packet = solid_padded <0, 0, 0, typename Fields::underlying..., solid_padded_terminal> ::result;
	using type = pad_tuple_conversion_t <packet>;
};

template <aggregate T>
struct solid_padded_builder <T> {
	using layout = decltype(T().layout());
	using type = typename solid_padded_builder <layout> ::type;
};

// One liner shortcut
template <generic T>
using solid_t = typename solid_padded_builder <T> ::type;

} // namespace jvl::ire