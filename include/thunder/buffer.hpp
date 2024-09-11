#pragma once

#include "atom.hpp"
#include "qualified_type.hpp"
#include "../wrapped_types.hpp"

namespace jvl::thunder {

// Forward declarations
struct Kernel;
struct QualifiedType;

// Arbitrary pools of atoms
struct Buffer {
	// TODO: mirror table with vector <index_t> as list
	std::vector <QualifiedType> types;
	std::vector <Atom> pool;
	size_t pointer;

	Buffer();

	index_t emit(const Atom &, bool = true);
	QualifiedType classify(index_t) const;
	Kernel export_to_kernel() const;
	void validate() const;
	void clear();
	void dump() const;

	// Utility methods
	Atom &operator[](size_t);
	const Atom &operator[](size_t) const;

	std::vector <index_t> expand_list(index_t) const;
	std::vector <QualifiedType> expand_list_types(index_t) const;
};

} // namespace jvl::thunder
