#pragma once

#include "atom.hpp"
#include "qualified_type.hpp"
#include "../wrapped_types.hpp"

namespace jvl::thunder {

// Forward declarations
struct Kernel;
struct QualifiedType;

// Arbitrary pools of atoms
class Buffer {
	// Determine the qualified type of an instruction
	QualifiedType classify(index_t) const;

	// Populate the synthesized set
	void include(index_t);
public:
	std::unordered_set <index_t> synthesized;
	std::vector <QualifiedType> types;
	std::vector <Atom> atoms;
	size_t pointer;

	Buffer();

	index_t emit(const Atom &, bool = true);
	Kernel export_to_kernel() const;
	void validate() const;
	void clear();
	void dump() const;

	// TODO: shrink to fit method

	// Utility methods
	Atom &operator[](size_t);
	const Atom &operator[](size_t) const;

	std::vector <index_t> expand_list(index_t) const;
	std::vector <QualifiedType> expand_list_types(index_t) const;
};

} // namespace jvl::thunder
