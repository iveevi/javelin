#pragma once

#include "atom.hpp"

namespace jvl::thunder {

// Forward declarations
struct Kernel;

// Arbitrary pools of atoms
struct Scratch {
	std::vector <Atom> pool;
	// TODO: mirror map with type references
	// TODO: mirror map with next references
	size_t pointer;

	Scratch();

	index_t emit(const Atom &);
	Kernel export_to_kernel() const;
	void validate() const;
	void clear();
	void dump();
};

} // namespace jvl::thunder
