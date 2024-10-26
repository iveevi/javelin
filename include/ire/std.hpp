#pragma once

#include "type_synthesis.hpp"
#include "native.hpp"

namespace jvl::ire {

// Optional types like std::optional
template <generic T>
struct optional {
	native_t <bool> status;
	promoted <T> data;
public:
	optional() : status(false) {}

	auto has_value() {
		return status;
	}

	auto value() {
		return data;
	}

	auto layout() const {
		return uniform_layout("optional",
			named_field(status),
			named_field(data));
	}

	void reset() {
		status = false;
	}
};

} // namespace jvl::ire