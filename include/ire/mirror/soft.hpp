#pragma once

#include "fragment_zipper.hpp"

namespace jvl::ire {

// Extracting and padding the element type from unsized arrays
template <solidifiable T>
struct align_for_array {
	using U = solid_t <T>;
	static constexpr size_t padding = meta::alignup(sizeof(U), 16) - sizeof(U);
	using type = padded_element <U, padding>;
};

template <solidifiable T>
auto convert_dynamic_element(unsized_array <T> *) -> align_for_array <T> ::type;

template <generic T>
auto convert_dynamic_element(T *) -> int
{
	static_assert(false, "unsolidifiable dynamic fields must be unsized arrays of solifidiable types");
	return int();
}

// Extracting dynamic elements and solid fields from aggregate layout
template <typename Packet, field_type ... Args>
struct layout_unzipper {};

template <typename Packet, field_type T, field_type ... Args>
struct layout_unzipper <Packet, T, Args...> {
	static auto eval() {
		using U = typename T::underlying;
		using Fail = std::pair <int, int>;

		if constexpr (solidifiable <U>) {
			if constexpr (sizeof...(Args) == 0) {
				static_assert(false,
					"field sequence is fully solidifiable, "
					"no need for soft inteface");
				return Fail();
			} else {
				using Appended = Packet::template push_back <T>;
				using Next = layout_unzipper <Appended, Args...>;
				return typename Next::type();
			}
		} else {
			if constexpr (sizeof...(Args) == 0) {
				using Element = decltype(convert_dynamic_element((U *) nullptr));
				return std::pair <Element, Packet> ();
			} else {
				static_assert(false,
					"unsolidifiable type must be the "
					"LAST element in the field sequence");
				return Fail();
			}
		}
	}

	// On success should evaluate to pair <Element, meta::type_packet <Fields...>>
	using type = decltype(eval());
};

template <bool Phantom, field_type ... Fields>
auto convert_layout_to_unzipper(Layout <Phantom, Fields...> *)
	-> layout_unzipper <meta::type_packet <>, Fields...>;

template <typename Element, field_type ... Fields>
auto convert_fragment_to_zipper(std::pair <Element, meta::type_packet <Fields...>> *)
	-> fragment_zipper <Element, Fields...>;

auto convert_pair_to_binder(std::pair <int, int> *)
	-> void;

template <aggregate T>
struct soft_builder {
	using layout = decltype(T().layout());
	using unzipper = decltype(convert_layout_to_unzipper((layout *) nullptr));
	using fragment = unzipper::type;
	using type = decltype(convert_fragment_to_zipper((fragment *) nullptr));
};

template <aggregate T>
using soft_t = typename soft_builder <T> ::type;

} // namespace jvl::ire