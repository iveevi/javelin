#pragma once

#include "buffer.hpp"

namespace jvl::thunder {

struct TrackedBuffer : Buffer {
	// Global list of callables
	static auto &tracked() {
		static std::unordered_map <size_t, TrackedBuffer *> map;
		return map;
	}

	static TrackedBuffer *search_tracked(size_t cid) {
		auto &t = tracked();
		if (t.contains(cid))
			return t[cid];

		return nullptr;
	}

	// Unique id
	size_t cid;

	// An optional name (defaults to "callable<cid>")
	std::string name;

	// For callables we can track back used and synthesized
	// insructions from working backwards at the returns

	TrackedBuffer();
	TrackedBuffer(const TrackedBuffer &);
	TrackedBuffer &operator=(const TrackedBuffer &);

	// TODO: destructor, which offloads it from the global list

	void dump() const;
};

} // namespace jvl::thunder