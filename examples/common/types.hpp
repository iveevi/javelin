#pragma once

#include <string>
#include <vector>
#include <map>

#include "math_types.hpp"
#include "wrapped_types.hpp"

namespace jvl {

// General key-value structure
template <typename T>
using property = jvl::wrapped::tree <std::string, T>;

// General buffer structures
using typed_vector = jvl::wrapped::variant <
	std::vector <float4>,
	std::vector <float3>,
	std::vector <float2>,
	std::vector <float>,
	std::vector <int3>,
	std::vector <int2>,
	std::vector <int4>,
	std::vector <int>
>;

// Size of buffers
inline size_t typed_buffer_size(const typed_vector &buffer)
{
	auto ftn = [](auto b) { return b.size(); };
	return std::visit(ftn, buffer);
}

} // namespace jvl
