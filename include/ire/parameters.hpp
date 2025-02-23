#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// TODO: use integer ids and aliases...
template <generic T>
struct in : T {
	in() = default;
	in(const T &other) : T(other) {}

	in &operator=(const T &other) {
		T ::operator=(other);
		return *this;
	}
};

template <generic T>
struct out : T {
	out() = default;
	out(const T &other) : T(other) {}

	out &operator=(const T &other) {
		T ::operator=(other);
		return *this;
	}
};

template <generic T>
struct inout : T {
	inout() = default;
	inout(const T &other) : T(other) {}

	inout &operator=(const T &other) {
		T ::operator=(other);
		return *this;
	}
};

} // namespace jvl::ire
