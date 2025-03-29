#pragma once

#include "atom.hpp"
#include "buffer.hpp"

namespace jvl::thunder {

// Usage graph
using UsageSet = std::set <Index>;
using UsageGraph = std::vector <UsageSet>;

// Structure for recording instruction transformations
struct ref_index_t {
	Index index;
	int8_t mask = 0b11;
};

struct mapped_instruction_t : Buffer {
	std::vector <ref_index_t> refs;

	void track(Index index, int8_t mask = 0b11) {
		refs.push_back(ref_index_t(index, mask));
	}
};

// Atom usage dependency retrieval
UsageSet usage(const std::vector <Atom> &, Index);
UsageSet usage(const Buffer &, Index);
UsageGraph usage(const Buffer &);

} // namespace jvl::thunder