#pragma once

#include "resources.hpp"

namespace jvl::rexec {

//////////////////////////////////////
// Resource collections and filters //
//////////////////////////////////////

template <resource ... Resources>
struct resource_collection {
	template <resource ... More>
	using append_front = resource_collection <Resources..., More...>;
};

// Filtering resources
#define DEFINE_RESOURCE_FILTER(concept_suffix)								\
	template <typename T>										\
	struct filter_##concept_suffix {								\
		using group = void;									\
	};												\
													\
	template <>											\
	struct filter_##concept_suffix <resource_collection <>> {					\
		using group = resource_collection <>;							\
	};												\
													\
	template <resource R, resource ... Rest>							\
	struct filter_##concept_suffix <resource_collection <R, Rest...>> {				\
		using upper = filter_##concept_suffix <resource_collection <Rest...>> ::group;		\
		using fixed = typename upper::template append_front <R>;				\
		using group = std::conditional_t <resource_##concept_suffix <R>, fixed, upper>;		\
	};

DEFINE_RESOURCE_FILTER(layout_in)
DEFINE_RESOURCE_FILTER(layout_out)
DEFINE_RESOURCE_FILTER(push_constant)

} // namespace jvl::rexec