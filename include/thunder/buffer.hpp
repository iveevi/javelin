#pragma once

#include <set>

#include "atom.hpp"
#include "qualified_type.hpp"

namespace jvl::thunder {

// Forward declarations
struct QualifiedType;

// Arbitrary pools of atoms
class Buffer {
	void transfer_decorations(Index, Index);

	// Determine the qualified type of an instruction
	QualifiedType classify_qualifier(const Qualifier &, Index);
	QualifiedType classify(Index);

	// Populate the synthesized set
	void include(Index);
public:
	struct type_hint {
		std::string name;
		std::vector <std::string> fields;
	};

	size_t pointer;
	std::map <thunder::Index, uint64_t> used_decorations;
	std::map <uint64_t, type_hint> decorations;
	std::set <Index> synthesized;
	std::vector <Atom> atoms;
	std::vector <QualifiedType> types;

	Buffer();

	Index emit(const Atom &, bool = true);

	void clear();
	void dump() const;

	// TODO: shrink to fit method

	// Utility methods
	Atom &operator[](size_t);
	const Atom &operator[](size_t) const;

	std::vector <Index> expand_list(Index) const;
	std::vector <QualifiedType> expand_list_types(Index) const;
};

} // namespace jvl::thunder
