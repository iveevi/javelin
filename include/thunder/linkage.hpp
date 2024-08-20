#pragma once

#include <vector>

#include "atom.hpp"

namespace jvl::thunder {

// Linkage requirements of a kernel
struct Linkage {
	// TODO: "precompiled" units that are cached (instead of relinking everything)
	struct struct_element {
		PrimitiveType item = PrimitiveType::bad;

		// Should be (-1) if not nested
		index_t nested = -1;
	};

	using struct_declaration = std::vector <struct_element>;

	// Callable functions used
	std::unordered_set <index_t> callables;

	// Types for global variables
	wrapped::hash_table <index_t, index_t> lins;
	wrapped::hash_table <index_t, index_t> louts;

	// By default assume no push constants
	index_t push_constant = -1;

	// Block information
	using struct_map_t = wrapped::hash_table <index_t, index_t>;
	using source_block_t = std::vector <Atom>;

	struct block_t {
		wrapped::hash_table <index_t, index_t> parameters;
		std::unordered_set <index_t> synthesized;
		struct_map_t struct_map;
		source_block_t unit;
		index_t returns = -1;
	};

	wrapped::hash_table <index_t, block_t> blocks;

	// Separated list of unique structs
	std::vector <struct_declaration> structs;

	// Topologically sorted blocks
	std::vector <index_t> sorted;

	// Adding new struct declarations uniquely
	index_t insert(const struct_declaration &);

	// Recursively resolving all linkage information
	Linkage &resolve();

	// Synthesizing the final output
	std::string synthesize_glsl(const std::string &);

	// Printing linkage state
	void dump() const;
};

} // namespace jvl::atom
