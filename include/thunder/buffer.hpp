#pragma once

#include <filesystem>
#include <set>

#include "atom.hpp"
#include "qualified_type.hpp"

namespace jvl::thunder {

// Forward declarations
struct QualifiedType;

// Arbitrary pools of atoms
class Buffer {
	void transfer_decorations(Index, Index);

	// Semantic analysis for atoms
	QualifiedType semalz_qualifier(const Qualifier &, Index);
	QualifiedType semalz_construct(const Construct &, Index);
	QualifiedType semalz_load(const Load &, Index);
	QualifiedType semalz_access(const ArrayAccess &, Index);
	QualifiedType semalz(Index);

	// Populate the synthesized set
	bool naturally_forced(const Atom &);
	
	void mark_children(Index);
	void mark(Index, bool = false);
public:
	struct type_hint {
		std::string name;
		std::vector <std::string> fields;
	};

	size_t pointer;
	std::set <Index> synthesized;
	std::vector <Atom> atoms;
	std::vector <QualifiedType> types;
	
	struct {
		std::map <uint64_t, type_hint> all;
		std::map <Index, uint64_t> used;
		// TODO: vector <hint> and variant for hints... or multimap?
		std::set <Index> phantom;
	} decorations;

	Buffer();

	Index emit(const Atom &, bool = true);

	void clear();
	
	// Debugging and visualization utilities
	void display_assembly() const;
	void display_pretty() const;
	void graphviz(const std::filesystem::path &) const;

	// TODO: shrink to fit method

	// Utility methods
	Atom &operator[](size_t);
	const Atom &operator[](size_t) const;

	std::vector <Index> expand_list(Index) const;
	std::vector <QualifiedType> expand_list_types(Index) const;
};

#define JVL_BUFFER_DUMP_ON_ASSERT(cond, ...)	\
	do {					\
		if (!cond)			\
			display_pretty();	\
		JVL_ASSERT(cond, __VA_ARGS__);	\
	} while (0)

#define JVL_BUFFER_DUMP_AND_ABORT(...)		\
	do {					\
		display_pretty();		\
		JVL_ABORT(__VA_ARGS__);		\
	} while(0)

} // namespace jvl::thunder
