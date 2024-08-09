#pragma once

#include <string>
#include <vector>

#include "math_types.hpp"
#include "wrapped_types.hpp"

namespace jvl {

template <typename T>
using buffer = std::vector <T>;

template <typename T>
using property = jvl::wrapped::hash_table <std::string, T>;

using typed_buffer = jvl::wrapped::variant <
	buffer <float3>, buffer <float2>,
	buffer <int3>, buffer <int4>
>;

} // namespace jvl
