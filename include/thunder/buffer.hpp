#pragma once

#include "atom.hpp"
#include "enumerations.hpp"
#include "../wrapped_types.hpp"

namespace jvl::thunder {

// Forward declarations
struct Kernel;

// Type declaration reference
struct TypeDecl {
	PrimitiveType primitive = bad;
	index_t concrete = -1;
	// TODO: qualifiers

	TypeDecl() = default;
	TypeDecl(const PrimitiveType type) : primitive(type) {}
	TypeDecl(const index_t ref) : concrete(ref) {}

	operator bool() {
		return (primitive != bad) || (concrete != -1);
	}

	bool operator==(const TypeDecl &other) const {
		return (primitive == other.primitive)
			&& (concrete == other.concrete);
	}

	// TODO: source code
	std::string to_string() const {
		if (primitive != bad)
			return tbl_primitive_types[primitive];
		else if (concrete != -1)
			return fmt::format("concrete(%{})", concrete);
		return "nil";
	}
};

// Arbitrary pools of atoms
struct Buffer {
	wrapped::hash_table <index_t, TypeDecl> types;
	std::vector <Atom> pool;
	size_t pointer;

	Buffer();

	index_t emit(const Atom &);
	TypeDecl classify(const Atom &) const;
	Kernel export_to_kernel() const;
	void validate() const;
	void clear();
	void dump();

	// Utility methods
	Atom &operator[](size_t);
	const Atom &operator[](size_t) const;

	std::vector <index_t> expand_list(index_t) const;
	std::vector <TypeDecl> expand_list_types(index_t) const;
};

} // namespace jvl::thunder
