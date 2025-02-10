#pragma once

#include <map>
#include <set>

#include <bestd/optional.hpp>

// Wrappers on standard types
namespace jvl {

// Reindexing map
template <typename T>
requires std::is_integral_v <T>
struct reindex : std::map <T, T> {
	reindex() : std::map <T, T> { { -1, -1 } } {}

	int mapped(T i) const {
		if (this->contains(i))
			return this->at(i);

		return -1;
	}

	void operator()(T &i) const {
		if (this->contains(i))
			i = this->at(i);
		else
			i = -1;
	}

	std::set <T> operator()(std::set <T> &set) {
		std::set <T> replace;
		for (const T &i : set)
			replace.insert(mapped(i));

		return replace;
	}
};

} // namespace jvl