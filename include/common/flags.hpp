#pragma once

// Generating default operations for enumeration classes
#define flag_operators(T, base)						\
	constexpr T operator+(T A, T B)					\
	{								\
		return T(base(A) | base(B));				\
	}								\
	constexpr bool has(T A, T B)					\
	{								\
		return ((base(A) & base(B)) == base(B));		\
	}
