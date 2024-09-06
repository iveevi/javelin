#pragma once

#include <vector>

#include "atom.hpp"
#include "../ire/scratch.hpp"

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

	struct block_t {
		std::string name;
		wrapped::hash_table <index_t, index_t> parameters;
		struct_map_t struct_map;
		std::vector <Atom> unit;
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

	// Generate GLSL source code
	// TODO: pass integer number... or enum
	std::string generate_glsl(const std::string &);

	// Generate C++ source code
	std::string generate_cplusplus();

	// Synthesizing binary code...
	struct jit_result_t {
		void *result;
	};

	// ..through libgccjit
	jit_result_t generate_jit_gcc();

	// Printing linkage state
	void dump() const;
};

// Helper functions for code generation
namespace detail {

// Determine the set of instructions to concretely synthesized
std::unordered_set <index_t> synthesize_list(const std::vector <Atom> &);

// For C-like languages
std::string generate_body_c_like(const std::vector <Atom> &,
		                 const wrapped::hash_table <int, std::string> &,
		                 const std::unordered_set <index_t> &,
				 size_t);

// Legalization methods
void legalize_for_cc(ire::Scratch &);

}

} // namespace jvl::thunder
