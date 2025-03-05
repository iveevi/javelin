#pragma once

#include <map>

#include "buffer.hpp"

namespace jvl::thunder {

// Buffer with name
struct NamedBuffer : Buffer {
	std::string name;
};

// Buffer with name and unique index
struct TrackedBuffer : NamedBuffer {
	// Global list of callables
	struct cache_entry_t {
		int32_t count;
		NamedBuffer buffer;
		const NamedBuffer *link;
	};

	using cache_t = std::map <int32_t, cache_entry_t>;

	static cache_t &cache();
	static const NamedBuffer &cache_load(int32_t);
	static void cache_increment(int32_t);
	static void cache_decrement(int32_t);
	static void cache_unlink(int32_t);
	static void cache_insert(const TrackedBuffer *);
	static void cache_display();

	// Unique id
	int32_t cid;

	TrackedBuffer();
	TrackedBuffer(const TrackedBuffer &);
	TrackedBuffer &operator=(const TrackedBuffer &);
	~TrackedBuffer();

	void display_assembly() const;
	void display_pretty() const;
};

} // namespace jvl::thunder