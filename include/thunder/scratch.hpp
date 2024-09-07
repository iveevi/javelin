#pragma once

#include "atom.hpp"

namespace jvl::thunder {

// Arbitrary pools of atoms
struct Scratch {
	std::vector <Atom> pool;
	// TODO: mirror map with type references
	// TODO: mirror map with next references
	size_t pointer;

	Scratch();

	void reserve(size_t);

	index_t emit(const Atom &);

	void clear();

	void dump();
};

} // namespace jvl::thunder
