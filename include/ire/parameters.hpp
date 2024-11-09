#pragma once

#include "type_system.hpp"

namespace jvl::ire {

template <generic T>
struct in : promoted <T> {
	in() = default;
	in(const promoted <T> &other) : promoted <T> (other) {}

	in &operator=(const promoted <T> &other) {
		promoted <T> ::operator=(other);
		return *this;
	}
};

template <generic T>
struct out : promoted <T> {
	out() = default;
	out(const promoted <T> &other) : promoted <T> (other) {}

	out &operator=(const promoted <T> &other) {
		promoted <T> ::operator=(other);
		return *this;
	}
};

template <generic T>
struct inout : promoted <T> {
	inout() = default;
	inout(const promoted <T> &other) : promoted <T> (other) {}

	inout &operator=(const promoted <T> &other) {
		promoted <T> ::operator=(other);
		return *this;
	}
};

} // namespace jvl::ire
