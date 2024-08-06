#pragma once

#include <cstdlib>

namespace jvl::ire {

// Interface
struct emit_index_t {
	using value_type = int;

	value_type id;

	emit_index_t &operator=(const value_type &v) {
		id = v;
		return *this;
	}

	static emit_index_t null() {
		return {.id = -1};
	}

	bool operator==(const value_type &v) {
		return id == v;
	}

	bool operator==(const emit_index_t &c) {
		return id == c.id;
	}
};

struct tagged {
	mutable emit_index_t ref;
	mutable bool immutable;

	tagged(emit_index_t r = emit_index_t::null())
			: ref(r), immutable(false) {}

	[[gnu::always_inline]]
	bool cached() const {
		return ref != -1;
	}

	int synthesize() const {
		if (!cached())
			abort();

		return ref.id;
	}
};

} // namespace jvl::ire
