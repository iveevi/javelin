#pragma once

#include <map>

// Wrappers on standard types
namespace jvl::thunder {

// Reindexing map
template <std::integral T>
struct reindex : std::map <T, T> {
	reindex() {}

	void operator()(T &i) const {
		if (this->contains(i))
			i = this->at(i);
	}
};

} // namespace jvl::thunder