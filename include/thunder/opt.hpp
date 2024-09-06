#pragma once

#include "atom.hpp"
#include "../ire/scratch.hpp"

namespace jvl::thunder {

// Usage graph
using usage_set = std::unordered_set <index_t>;
using usage_graph = std::vector <usage_set>;

// Structure for recording instruction transformations
struct ref_index_t {
	index_t index;
	int8_t mask = 0b11;
};

struct mapped_instruction_t {
	ire::Scratch transformed;
	std::vector <ref_index_t> refs;

	Atom &operator[](size_t index) {
		return transformed.pool[index];
	}

	void track(index_t index, int8_t mask = 0b11) {
		refs.push_back(ref_index_t(index, mask));
	}
};

// Atom usage dependency retrieval
usage_set usage(const std::vector <Atom> &, index_t);
usage_set usage(const ire::Scratch &, index_t);
usage_graph usage(const ire::Scratch &);

// Optimization transformation passes
bool opt_transform_compact(ire::Scratch &);
bool opt_transform_constructor_elision(ire::Scratch &);
bool opt_transform_dead_code_elimination(ire::Scratch &);

// Full optimization pass
// TODO: options to control level of optimization
void opt_transform(ire::Scratch &);

// Stitching mapped instruction blocks
void stitch_mapped_instructions(ire::Scratch &, std::vector <mapped_instruction_t> &);

} // namespace jvl::thunder
