#pragma once

#include <fmt/printf.h>
#include <fmt/format.h>

#include <bestd/variant.hpp>

#include "../core/reindex.hpp"
#include "../core/logging.hpp"
#include "enumerations.hpp"

namespace jvl::thunder {

// TODO: docs generator for atoms

// Index type, small to create compact IR
using Index = int16_t;

// Addresses referenced in an instruction,
// useful for various reindexing operations
struct Addresses {
	Index &a0;
	Index &a1;

	static Index &null() {
		MODULE(atom-addresses);

		static thread_local Index null = -1;
		JVL_ASSERT(null == -1, "address null is invalid");
		return null;
	}
};

// Qualifier for an underlying type
//
//   underlying: reference to the underlying type
//   numerical: possibly a binding location, array count, etc.
//   qualifier: qualifier type for the global variable/parameter
struct Qualifier {
	Index underlying = -1;
	Index numerical = -1;
	QualifierKind kind;

	bool operator==(const Qualifier &) const;

	Addresses addresses();

	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Type information, including primitives and structs
//
//   down: if valid (!= -1), points to a nested struct
//   next: if valid (!= -1), points to the next field in the struct
//   item: if valid (!= BAD), indicates the primitive type of the current field
struct TypeInformation {
	Index down = -1;
	Index next = -1;
	PrimitiveType item = bad;

	bool operator==(const TypeInformation &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Primitive value
//
//   bdata: boolean value
//   fdata: floating point value
//   idate: integer value
//   type: type of the primitive (limited)
#pragma pack(push, 1)
struct Primitive {
	union {
		bool bdata;
		uint32_t udata;
		int32_t idata;
		float fdata;
	};

	PrimitiveType type = bad;

	bool operator==(const Primitive &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
	std::string value_string() const;
};
#pragma pack(pop)

// Swizzle instruction
//
//   src: reference to the value to swizzle
//   code: swizzle index (SwizzleCode)
struct Swizzle {
	Index src = -1;
	SwizzleCode code;

	bool operator==(const Swizzle &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Operation instruction
//
//   a: reference to the first argument
//   b: reference to the second argument
//   code: operation type (OperationCode)
struct Operation {
	Index a = -1;
	Index b = -1;
	OperationCode code;

	bool operator==(const Operation &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Intrinsic instruction, for invoking platform intrinsics
//
//   args: reference to a List chain of operands
//   type: return type of the instrinsic
//   opn: intrinsic operation code
struct Intrinsic {
	Index args = -1;
	IntrinsicOperation opn;

	bool operator==(const Intrinsic &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// List chain node
//
//   item: if valid, points to the value of the node
//   next: if valid, points to the next List chain node
struct List {
	Index item = -1;
	Index next = -1;

	bool operator==(const List &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Constructing a (complex) primitive or composite type
//
//   type: type to construct
//   args: reference to a List chain of arguments
//   mode: construction mode
struct Construct {
	Index type = -1;
	Index args = -1;
	ConstructorMode mode = normal;

	bool operator==(const Construct &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Invoking a callable function (subroutine)
//
//   cid: unique index of the callable
//   args: reference to a List chain of arguments
//   type: return type of the callable
struct Call {
	Index cid = -1;
	Index args = -1;
	Index type = -1;

	bool operator==(const Call &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Store instruction
//
//   dst: reference to store into
//   src: reference to the value
struct Store {
	Index dst = -1;
	Index src = -1;

	bool operator==(const Store &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Load instruction, with fields
//
//   src: reference to the value to load from
//   idx: field index of the value, unless invalid (==-1)
struct Load {
	Index src = -1;
	Index idx = -1;

	bool operator==(const Load &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Accessing elements of arrays
//
//   src: reference to the value to load from
//   loc: reference to the index of the access
struct ArrayAccess {
	Index src = -1;
	Index loc = -1;
	
	bool operator==(const ArrayAccess &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Branching instruction
//
//   cond: reference to the condition value
//   failto: reference to the jump address (for failure)
struct Branch {
	Index cond = -1;
	Index failto = -1;
	BranchKind kind = conditional_if;

	bool operator==(const Branch &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Returning values from subroutines and kernels
//
//   value: reference to the value to be returned
struct Returns {
	Index value = -1;

	bool operator==(const Returns &) const;

	Addresses addresses();
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Atom instructions
using atom_base = bestd::variant <
	Qualifier,
	TypeInformation,
	Primitive,
	Swizzle,
	Operation,
	Intrinsic,
	List,
	Construct,
	Call,
	Store,
	Load,
	ArrayAccess,
	Branch,
	Returns
>;

struct alignas(4) Atom : atom_base {
	using atom_base::atom_base;

	Addresses addresses();

	void reindex(const reindex <Index> &);
	
	std::string to_assembly_string() const;
	std::string to_pretty_string() const;
};

// Support for direct printing through fmt
std::string format_as(const Atom &atom);

// Atom size checks
static_assert(sizeof(Qualifier)		== 6);
static_assert(sizeof(TypeInformation)	== 6);
static_assert(sizeof(Primitive)		== 6);
static_assert(sizeof(Swizzle)		== 4);
static_assert(sizeof(Operation)		== 6);
static_assert(sizeof(Intrinsic)		== 4);
static_assert(sizeof(List)		== 4);
static_assert(sizeof(Construct)		== 6);
static_assert(sizeof(Call)		== 6);
static_assert(sizeof(Store)		== 4);
static_assert(sizeof(Load)		== 4);
static_assert(sizeof(Branch)		== 6);
static_assert(sizeof(Returns)		== 2);
static_assert(sizeof(Atom)		== 8);

} // namespace jvl::thunder
