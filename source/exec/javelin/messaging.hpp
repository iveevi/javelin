#pragma once

#include <cstdint>

struct UUID {
	int64_t typed;
	int64_t global;
};

[[gnu::always_inline]]
inline int64_t new_global_uuid()
{
	static int64_t uuid = 0;
	return uuid++;
}

template <typename T>
[[gnu::always_inline]]
inline int64_t new_typed_uuid()
{
	static int64_t uuid = 0;
	return uuid++;
}

template <typename T>
[[gnu::always_inline]]
inline UUID new_uuid()
{
	return UUID {
		.typed = new_typed_uuid <T> (),
		.global = new_global_uuid()
	};
}

// TODO: UUID based messaging system