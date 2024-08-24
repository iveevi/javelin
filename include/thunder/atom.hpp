#pragma once

#include <cassert>
#include <unordered_set>
#include <vector>

#include <fmt/printf.h>

#include "enumerations.hpp"
#include "../wrapped_types.hpp"

namespace jvl::thunder {

// Index type, small to create compact IR
using index_t = int16_t;

// Addresses referenced in an instruction,
// useful for a variety of reindexing operations
struct Addresses {
	index_t &a0;
	index_t &a1;

	static index_t &null() {
		static thread_local index_t null = -1;
		assert(null == -1);
		return null;
	}
};

// Interface to enforce guarantees about operations,
// useful when enforcing decisions to change the layout
template <typename T>
concept atom_instruction = requires(T &t) {
	{
		t.addresses()
	} -> std::same_as <Addresses>;
};

// TODO: pack everything, manually pad to align

// Global variable or parameter:
//
//   type: reference to the type of the variable/parameter
//   binding: binding location, if applicable
//   qualifier: qualifier type for the global variable/parameter
struct Global {
	index_t type = -1;
	index_t binding = -1;
	GlobalQualifier qualifier;

	Addresses addresses() {
		return { type, Addresses::null() };
	}
};

static_assert(atom_instruction <Global>);

// Struct field of a uniform compatible type
//
//   down: if valid (!= -1), points to a nested struct
//   next: if valid (!= -1), points to the next field in the struct
//   item: if valid (!= BAD), indicates the primitive type of the current field
struct TypeField {
	index_t down = -1;
	index_t next = -1;
	PrimitiveType item = bad;

	Addresses addresses() {
		return { down, next };
	}
};

static_assert(atom_instruction <TypeField>);

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
		int idata;
		float fdata;
	};

	PrimitiveType type = bad;
	
	Addresses addresses() {
		return { Addresses::null(), Addresses::null() };
	}
};
#pragma pack(pop)

static_assert(atom_instruction <Primitive>);

// Swizzle instruction
//
//   src: reference to the value to swizzle
//   code: swizzle index (SwizzleCode)
struct Swizzle {
	index_t src = -1;
	SwizzleCode code;

	Addresses addresses() {
		return { src, Addresses::null() };
	}
};

static_assert(atom_instruction <Swizzle>);

// TODO: specialize into binary args (second could be null),
// and type should be a primitive type (int8_t)

// Operation instruction
//
//   args: reference to a List chain of operands
//   type: return type of the operation
//   code: operation type (OperationCode)
struct Operation {
	index_t args = -1;
	index_t type = -1;
	OperationCode code;

	Addresses addresses() {
		return { args, type };
	}
};

static_assert(atom_instruction <Operation>);

// Intrinsic instruction, for invoking platform intrinsics
//
//   args: reference to a List chain of operands
//   type: return type of the instrinsic
//   opn: intrinsic operation code
struct Intrinsic {
	index_t args = -1;
	index_t type = -1;
	IntrinsicOperation opn;

	Addresses addresses() {
		return { args, type };
	}
};

static_assert(atom_instruction <Intrinsic>);

// List chain node
//
//   item: if valid, points to the value of the node
//   next: if valid, points to the next List chain node
struct List {
	index_t item = -1;
	index_t next = -1;

	Addresses addresses() {
		return { item, next };
	}
};

static_assert(atom_instruction <List>);

// Constructing a (complex) primitive or composite type
//
//   type: type to construct
//   args: reference to a List chain of arguments
struct Construct {
	index_t type = -1;
	index_t args = -1;

	Addresses addresses() {
		return { type, args };
	}
};

static_assert(atom_instruction <Construct>);

// Invoking a callable function (subroutine)
//
//   cid: unique index of the callable
//   args: reference to a List chain of arguments
//   type: return type of the callable
struct Call {
	index_t cid = -1;
	index_t args = -1;
	index_t type = -1;

	Addresses addresses() {
		return { args, type };
	}
};

// Store instruction
//
//   dst: reference to store into
//   src: reference to the value
struct Store {
	index_t dst = -1;
	index_t src = -1;

	Addresses addresses() {
		return { dst, src };
	}
};

static_assert(atom_instruction <Store>);

// Load instruction, with fields
//
//   src: reference to the value to load from
//   idx: field index of the value, unless invalid (==-1)
struct Load {
	index_t src = -1;
	index_t idx = -1;

	Addresses addresses() {
		return { src, Addresses::null() };
	}
};

static_assert(atom_instruction <Load>);

// Branching instruction
//
//   cond: reference to the condition value
//   failto: reference to the jump address (for failure)
struct Branch {
	index_t cond = -1;
	index_t failto = -1;

	Addresses addresses() {
		return { cond, failto };
	}
};

static_assert(atom_instruction <Branch>);

// Looping instruction
//
//   cond: reference to the condition value
//   failto: reference to the jump address (for failure)
struct While {
	index_t cond = -1;
	index_t failto = -1;
	
	Addresses addresses() {
		return { cond, failto };
	}
};

static_assert(atom_instruction <While>);

// Returning values from subroutines and kernels
//
//   args: reference to List chain of return values
//   type: return type
struct Returns {
	index_t args = -1;
	index_t type = -1;

	Addresses addresses() {
		return { args, type };
	}
};

static_assert(atom_instruction <Returns>);

// Target address of a branch/loop failto index
// TODO: remove end
struct End {
	Addresses addresses() {
		return { Addresses::null(), Addresses::null() };
	}
};

static_assert(atom_instruction <End>);

// Atom instructions
using atom_base = wrapped::variant <
	Global,
	TypeField,
	Primitive,
	Swizzle,
	Operation,
	Construct,
	Call,
	Intrinsic,
	List,
	Store,
	Load,
	Branch,
	While,
	Returns,
	End
>;

struct alignas(4) Atom : atom_base {
	using atom_base::atom_base;

	Addresses addresses() {
		auto ftn = [](auto &x) -> Addresses { return x.addresses(); };
		return std::visit(ftn, *this);
	}
};

static_assert(atom_instruction <Atom>);

// TODO: move to guarantees.hpp
static_assert(sizeof(Global)    == 6);
static_assert(sizeof(TypeField) == 6);
static_assert(sizeof(Primitive) == 5);
static_assert(sizeof(Swizzle)   == 4);
static_assert(sizeof(Operation) == 6);
static_assert(sizeof(Intrinsic) == 6);
static_assert(sizeof(List)      == 4);
static_assert(sizeof(Construct) == 4);
static_assert(sizeof(Call)      == 6);
static_assert(sizeof(Store)     == 4);
static_assert(sizeof(Load)      == 4);
static_assert(sizeof(Branch)    == 4);
static_assert(sizeof(While)     == 4);
static_assert(sizeof(Returns)   == 4);
static_assert(sizeof(End)       == 1);
static_assert(sizeof(Atom)      == 8);

// Dispatcher types (vd = variadic dispatcher)
std::string type_name(const std::vector <Atom> &,
		      const wrapped::hash_table <int, std::string> &,
		      index_t, index_t);

// Printing instructions for debugging
void dump_ir_operation(const Atom &);

// Reindexing integer elements during compaction
void reindex_ir_operation(const wrapped::reindex <index_t> &, Atom &);

// Synthesizing GLSL source code
std::string synthesize_glsl_body(const std::vector <Atom> &,
		                 const wrapped::hash_table <int, std::string> &,
		                 const std::unordered_set <index_t> &,
				 size_t);

} // namespace jvl::thunder
