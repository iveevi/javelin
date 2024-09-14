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
	buffer <float4>,
	buffer <float3>,
	buffer <float2>,
	buffer <float>,
	buffer <int3>,
	buffer <int2>,
	buffer <int4>,
	buffer <int>
>;

inline size_t typed_buffer_size(const typed_buffer &buffer)
{
	auto ftn = [](auto b) { return b.size(); };
	return std::visit(ftn, buffer);
}

} // namespace jvl
