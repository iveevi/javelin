#pragma once

#include <unordered_set>
#include <vector>

#include "../profiles/targets.hpp"
#include "atom.hpp"
#include "buffer.hpp"
#include "enumerations.hpp"

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
	struct layout_info {
		index_t type;
		QualifierKind kind;
	};

	wrapped::hash_table <index_t, layout_info> lins;
	wrapped::hash_table <index_t, layout_info> louts;
	wrapped::hash_table <index_t, QualifierKind> samplers;

	// By default assume no push constants
	index_t push_constant = -1;

	// Block information
	using struct_map_t = wrapped::hash_table <index_t, index_t>;

	struct block_t : Buffer {
		std::string name;

		wrapped::hash_table <index_t, index_t> parameters;
		struct_map_t struct_map;
		index_t returns = -1;

		block_t() = default;
		block_t(const Buffer &buffer, const std::string &s)
			: Buffer(buffer), name(s) {}
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
	std::string generate_glsl(const profiles::glsl_version &);

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

// Legalization methods
void legalize_for_cc(Buffer &);

}

} // namespace jvl::thunder
