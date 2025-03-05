#pragma once

#include "atom.hpp"
#include "buffer.hpp"

namespace jvl::thunder {

// Usage graph
using usage_set = std::set <Index>;
using usage_graph = std::vector <usage_set>;

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
usage_set usage(const std::vector <Atom> &, Index);
usage_set usage(const Buffer &, Index);
usage_graph usage(const Buffer &);

} // namespace jvl::thunder