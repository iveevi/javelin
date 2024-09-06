#pragma once

#include "../thunder/atom.hpp"

namespace jvl::ire {

// Arbitrary pools of atoms
struct Scratch {
	using index_t = thunder::index_t;

	std::vector <thunder::Atom> pool;
	// TODO: mirror buffer with type references
	size_t pointer;

	// TODO: buffering with a static sized array

	Scratch();

	void reserve(size_t);

	index_t emit(const thunder::Atom &);

	void clear();

	void dump();
};

} // namespace jvl::ire
