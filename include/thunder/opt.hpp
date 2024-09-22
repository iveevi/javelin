#pragma once

#include "atom.hpp"
#include "buffer.hpp"

namespace jvl::thunder {

// Usage graph
using usage_set = std::unordered_set <index_t>;
using usage_graph = std::vector <usage_set>;

// Structure for recording instruction transformations
struct ref_index_t {
	index_t index;
	int8_t mask = 0b11;
};

struct mapped_instruction_t : Buffer {
	std::vector <ref_index_t> refs;

	void track(index_t index, int8_t mask = 0b11) {
		refs.push_back(ref_index_t(index, mask));
	}
};

// Atom usage dependency retrieval
usage_set usage(const std::vector <Atom> &, index_t);
usage_set usage(const Buffer &, index_t);
usage_graph usage(const Buffer &);

// Optimization transformation passes
// TODO: store elision
bool opt_transform_compact(Buffer &);
bool opt_transform_constructor_elision(Buffer &);
bool opt_transform_dead_code_elimination(Buffer &);

// Full optimization pass
// TODO: options to control level of optimization
void opt_transform(Buffer &);

// Legalizing instructions for C-family compiled targets
void legalize_for_cc(Buffer &);

// Stitching mapped instruction blocks
void stitch_mapped_instructions(Buffer &, std::vector <mapped_instruction_t> &);

} // namespace jvl::thunder
