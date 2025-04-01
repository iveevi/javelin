#pragma once

#include "collection.hpp"

namespace jvl::rexec {

///////////////////////////
// Resource compatiblity //
///////////////////////////

template <typename Super, typename Sub>
struct is_resource_subset : std::false_type {};

template <resource ... Supers, resource ... Subs>
struct is_resource_subset <
	resource_collection <Supers...>,
	resource_collection <Subs...>
> {
	template <resource S>
	static constexpr bool contained = (std::same_as <S, Supers> || ...);

	static constexpr bool value = (contained <Subs> && ...);
};

template <typename T, typename U>
concept resource_subset = is_resource_subset <T, U> ::value;

} // namespace jvl::rexec