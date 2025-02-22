#pragma once

#include <concepts>
#include <cstdlib>

namespace jvl::ire {

// Interface
struct cache_index_t {
	using value_type = int;

	// TODO: store scope address as well,
	// then we can track if something is wrong
	// or needs to be resynthesized...
	value_type id;

	cache_index_t &operator=(const value_type &v) {
		id = v;
		return *this;
	}

	bool operator==(const value_type &v) {
		return id == v;
	}

	bool operator==(const cache_index_t &c) {
		return id == c.id;
	}
	
	static cache_index_t from(value_type id) {
		cache_index_t cit;
		cit = id;
		return cit;
	}

	static cache_index_t null() {
		return { .id = -1 };
	}
};

// TODO: refactor cache_index_t -> Cached = bestd::optional <thunder::Index>

// TODO: use optional...
// TODO: rename to directly_synthesized(...)
struct tagged {
	// TODO: refactor to cache(...) method and idx() -> thunder::Index
	mutable cache_index_t ref;
	mutable bool immutable;

	tagged(cache_index_t r = cache_index_t::null())
			: ref(r), immutable(false) {}

	[[gnu::always_inline]]
	bool cached() const {
		return ref != -1;
	}

	cache_index_t synthesize() const {
		if (!cached())
			abort();

		cache_index_t cid;
		cid = ref.id;
		return cid;
	}
};

template <typename T>
concept builtin = requires(const T &t) {
	{
		t.synthesize()
	} -> std::same_as <cache_index_t>;
};

} // namespace jvl::ire
