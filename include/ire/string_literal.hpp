#pragma once

#include <algorithm>

namespace jvl::ire {

template <size_t N>
struct string_literal {
	static constexpr size_t length = N;
	
	constexpr string_literal(const char (&str)[N]) {
		std::copy_n(str, N, value);
	}

	char value[N];
};

template <size_t A, size_t B>
constexpr bool compare(const string_literal <A> &a, const string_literal <B> &b)
{
	if constexpr (A == B) {
		for (size_t i = 0; i < A; i++) {
			if (a.value[i] != b.value[i])
				return false;
		}

		return true;
	} else {
		return false;
	}
}

template <size_t I, size_t N, string_literal S, string_literal ... Args>
constexpr int32_t find_string_literal_impl(const string_literal <N> &s)
{
	if (compare(S, s))
		return I;

	if constexpr (sizeof...(Args)) {
		return find_string_literal_impl <I + 1, N, Args...> (s);
	} else {
		return -1;
	}
}

template <size_t N, string_literal ... Args>
constexpr int32_t find_string_literal(const string_literal <N> &s)
{
	return find_string_literal_impl <0, N, Args...> (s);
}

template <string_literal ... Args>
struct string_literal_group {
	template <size_t N>
	static constexpr int32_t find(const string_literal <N> &s) {
		return find_string_literal <N, Args...> (s);
	}
	
	template <size_t N>
	static constexpr int32_t find(const char (&s)[N]) {
		return find_string_literal <N, Args...> (string_literal <N> (s));
	}
};

} // namespace jvl::ire