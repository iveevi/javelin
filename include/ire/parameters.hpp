#pragma once

#include "type_system.hpp"

namespace jvl::ire {

template <generic T>
struct in : promoted <T> {
	in &operator=(const promoted <T> &other) {
		promoted <T> ::operator=(other);
		return *this;
	}
};

template <generic T>
struct out : promoted <T> {
	out &operator=(const promoted <T> &other) {
		promoted <T> ::operator=(other);
		return *this;
	}
};

template <generic T>
struct inout : promoted <T> {
	inout &operator=(const promoted <T> &other) {
		promoted <T> ::operator=(other);
		return *this;
	}
};

} // namespace jvl::ire
