#pragma once

#include <type_traits>
#include <cstdlib>

namespace jvl::ire {

// Interface
struct cache_index_t {
	using value_type = int;

	value_type id;

	cache_index_t &operator=(const value_type &v) {
		id = v;
		return *this;
	}

	static cache_index_t null() {
		return {.id = -1};
	}

	bool operator==(const value_type &v) {
		return id == v;
	}

	bool operator==(const cache_index_t &c) {
		return id == c.id;
	}
};

struct tagged {
	mutable cache_index_t ref;
	mutable bool immutable;

	tagged(cache_index_t r = cache_index_t::null())
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

template <typename T>
concept synthesizable = requires(const T &t) {
	{
		t.synthesize()
	} -> std::same_as <cache_index_t>;
};

} // namespace jvl::ire