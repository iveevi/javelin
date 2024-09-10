#pragma once

#include "atom.hpp"
#include "enumerations.hpp"
#include "../wrapped_types.hpp"

namespace jvl::thunder {

// Forward declarations
struct Kernel;

// Type declaration reference
// TODO: separate header
struct TypeDecl {
	PrimitiveType primitive = bad;
	index_t concrete = -1;
	index_t next = -1;
	// TODO: qualifiers

	TypeDecl() = default;

	TypeDecl(PrimitiveType primitive_, index_t next_ = -1)
		: primitive(primitive_), next(next_) {}

	TypeDecl(index_t concrete_, index_t next_ = -1)
		: concrete(concrete_), next(next_) {}

	bool is_primitive() const {
		return (primitive != bad);
	}

	bool is_struct_field() const {
		return (next != -1);
	}

	bool is_concrete() const {
		return (concrete != -1);
	}

	operator bool() {
		return (primitive != bad) || (concrete != -1);
	}

	bool operator==(const TypeDecl &other) const {
		return (primitive == other.primitive)
			&& (concrete == other.concrete);
	}

	// TODO: source code
	std::string to_string(bool full = false) const {
		std::string current;
		if (primitive != bad)
			current = tbl_primitive_types[primitive];
		else if (concrete != -1)
			current = fmt::format("concrete(%{})", concrete);
		else
			return "nil";

		if (!full || next == -1)
			return current;

		return fmt::format("{:15} :: next(%{})", current, next);
	}
};

// Arbitrary pools of atoms
struct Buffer {
	wrapped::hash_table <index_t, TypeDecl> types;
	std::vector <Atom> pool;
	size_t pointer;

	Buffer();

	index_t emit(const Atom &, bool = true);
	TypeDecl classify(index_t) const;
	Kernel export_to_kernel() const;
	void validate() const;
	void clear();
	void dump() const;

	// Utility methods
	Atom &operator[](size_t);
	const Atom &operator[](size_t) const;

	std::vector <index_t> expand_list(index_t) const;
	std::vector <TypeDecl> expand_list_types(index_t) const;
};

} // namespace jvl::thunder
