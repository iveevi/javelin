#include "thunder/tracked_buffer.hpp"

namespace jvl::thunder {

TrackedBuffer::TrackedBuffer() : Buffer()
{
	static size_t id = 0;
	cid = id++;
	name = fmt::format("callable{}", cid);

	tracked()[cid] = this;
}

TrackedBuffer::TrackedBuffer(const TrackedBuffer &other)
{
	*this = other;
}

// TODO: destructor will erase the entry if the active * == this

TrackedBuffer &TrackedBuffer::operator=(const TrackedBuffer &other)
{
	if (this != &other) {
		Buffer::operator=(other);
		cid = other.cid;
		name = other.name;

		// Replace with the most recent tracked version
		tracked()[cid] = this;
	}

	return *this;
}

void TrackedBuffer::dump() const
{
	fmt::println("------------------------------");
	fmt::println("TRACKED BUFFER ${} ({}/{})", name, pointer, atoms.size());
	fmt::println("------------------------------");
	Buffer::dump();
}

} // namespace jvl::thunder