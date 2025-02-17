#pragma once

#include "concepts.hpp"

namespace jvl::ire {

// Optional types like std::optional
template <generic T>
struct optional {
	// TODO: change to class
	native_t <bool> status;
	promoted <T> data;

	optional() : status(false) {}
	optional(std::nullopt_t) : status(false) {}
	optional(const T &value) : status(true), data(value) {}

	auto has_value() {
		return status;
	}

	auto value() {
		return data;
	}

	auto layout() {
		// TODO: prefix name by type
		return layout_from("Optional",
			verbatim_field(status),
			verbatim_field(data));
	}

	void reset() {
		status = false;
	}
};

} // namespace jvl::ire