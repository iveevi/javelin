#include "thunder/tracked_buffer.hpp"

namespace jvl::thunder {

MODULE(tracked-buffer);

// Global cache management
TrackedBuffer::cache_t &TrackedBuffer::cache()
{
	static cache_t cache;
	
	// JVL_INFO("cached buffers:");
	// for (auto &[k, entry] : cache) {
	// 	fmt::println("\t{} -> ({}, {}, {})",
	// 		k, entry.count,
	// 		entry.buffer.name,
	// 		(void *) entry.link);
	// }

	return cache;
}
	
const NamedBuffer &TrackedBuffer::cache_load(int32_t cid)
{
	auto &c = cache();
	if (c.contains(cid)) {
		auto &entry = c[cid];
		if (entry.link)
			return *entry.link;
		else
			return c[cid].buffer;
	}

	JVL_ABORT("no tracked buffer cache entry @{}", cid);
}

void TrackedBuffer::cache_increment(int32_t cid)
{
	auto &c = cache();
	c[cid].count++;
}

void TrackedBuffer::cache_decrement(int32_t cid)
{
	auto &c = cache();

	JVL_ASSERT(c.contains(cid), "no tracked buffer cache entry @{}", cid);
	if (--c[cid].count <= 0) {
		auto p = c[cid];
		JVL_INFO("offloading cache entry {} (@{})", p.buffer.name, cid);
		c.erase(cid);
	}
}

void TrackedBuffer::cache_unlink(int32_t cid)
{
	auto &c = cache();
	auto &entry = c[cid];

	// Transfer linked buffer, then "destroy" it
	if (entry.link) {
		entry.buffer = *entry.link;
		entry.link = nullptr;
	}
}

void TrackedBuffer::cache_insert(const TrackedBuffer *tb)
{
	auto &c = cache();
	c[tb->cid] = cache_entry_t(1, *tb, static_cast <const NamedBuffer *> (tb));
}

// Track buffer methods
TrackedBuffer::TrackedBuffer()
{
	static size_t id = 0;

	cid = id++;
	name = fmt::format("callable{}", cid);

	cache_insert(this);
}

TrackedBuffer::TrackedBuffer(const TrackedBuffer &other)
{
	*this = other;
}

TrackedBuffer &TrackedBuffer::operator=(const TrackedBuffer &other)
{
	if (this != &other) {
		NamedBuffer::operator=(other);

		// Replace with the most recent tracked version
		cid = other.cid;
		cache_increment(cid);
	}

	return *this;
}

TrackedBuffer::~TrackedBuffer()
{
	// Unlink; by now the buffer should have its contents
	cache_unlink(cid);
	
	cache_decrement(cid);
}

void TrackedBuffer::dump() const
{
	fmt::println("------------------------------");
	fmt::println("TRACKED BUFFER ${} ({}/{})", name, pointer, atoms.size());
	fmt::println("------------------------------");
	Buffer::dump();
}

} // namespace jvl::thunder
