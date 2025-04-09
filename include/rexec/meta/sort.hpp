#pragma once

#include "../resources.hpp"
#include "../collection.hpp"

namespace jvl::rexec {

// TODO: sort when filtering layout inputs?
template <resource_layout_in ... Ls>
struct sorted_layout_inputs {};

template <resource_layout_in Element, resource_layout_in ... Elements>
constexpr auto _sorted_layout_inputs_insert(const sorted_layout_inputs <Elements...> &)
{
	return sorted_layout_inputs <Element, Elements...> ();
}

template <resource_layout_in Element, typename List>
using sorted_layout_inputs_insert = decltype(_sorted_layout_inputs_insert <Element> (List()));

template <>
struct sorted_layout_inputs <> {
	template <resource_layout_in Element>
	using add = sorted_layout_inputs <Element>;
};

template <resource_layout_in Head, resource_layout_in ... Tail>
struct sorted_layout_inputs <Head, Tail...> {
	template <resource_layout_in Element>
	static constexpr auto _add() {
		if constexpr (Element::location < Head::location) {
			return sorted_layout_inputs <Element, Head, Tail...> ();
		} else {
			using fixed = sorted_layout_inputs <Tail...> ::template add <Element>;
			using complete = sorted_layout_inputs_insert <Head, fixed>;
			return complete();
		}
	}
	
	template <resource_layout_in Element>
	using add = decltype(_add <Element> ());
};

template <resource_layout_in ... Elements>
constexpr auto sort_layout_inputs(const resource_collection <Elements...> &);

template <resource_layout_in Head, resource_layout_in ... Tail>
constexpr auto sort_layout_inputs(const resource_collection <Head, Tail...> &)
{
	if constexpr (sizeof...(Tail)) {
		using next = decltype(sort_layout_inputs(resource_collection <Tail...> ()));
		return typename next::template add <Head> ();
	} else {
		return sorted_layout_inputs <Head> ();
	}
}

template <>
constexpr auto sort_layout_inputs <> (const resource_collection <> &)
{
	return sorted_layout_inputs <> ();
}

} // namespace jvl::rexec