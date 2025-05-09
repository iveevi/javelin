#pragma once

#include <cstdlib>

namespace jvl::meta {

template <class I>
constexpr I alignup(I x, size_t a)
{
	return I((x + (I(a) - 1)) & ~I(a - 1));
}

template <typename ... Args>
struct type_packet {
	template <typename T>
	using push_front = type_packet <T, Args...>;
};

} // namespace jvl::meta