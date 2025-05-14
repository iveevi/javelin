#pragma once

// Generating default operations for enumeration classes
#define DEFINE_FLAG_OPERATORS(T, base)					\
	constexpr T operator+(T A, T B)					\
	{								\
		return T(base(A) | base(B));				\
	}								\
	constexpr T operator-(T A, T B)					\
	{								\
		return T(base(A) & ~base(B));				\
	}								\
	constexpr bool has(T A, T B)					\
	{								\
		return ((base(A) & base(B)) == base(B));		\
	}
