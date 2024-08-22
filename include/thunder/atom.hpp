#pragma once

#include <fmt/printf.h>
#include <unordered_set>

#include "enumerations.hpp"
#include "../wrapped_types.hpp"

namespace jvl::thunder {

// Index type, small to create compact IR
using index_t = int16_t;

// Addresses referenced in an instruction
struct Addresses {
	index_t a0;
	index_t a1;
};

template <typename T>
concept __atom_instruction = requires(const T &t) {
	{
		t.addresses()
	} -> std::same_as <Addresses>;
};

// TODO: pack everything, manually pad to align

// Global variable or parameter:
//        [type] reference to the type of the variable/parameter
//     [binding] binding index, if applicable
//   [qualifier] qualifier type for the global variable/parameter
struct Global {
	index_t type = -1;
	index_t binding = -1;
	GlobalQualifier qualifier;

	Addresses addresses() const {
		return { type, -1 };
	}
};

static_assert(__atom_instruction <Global>);

struct TypeField {
	index_t down = -1;
	index_t next = -1;
	PrimitiveType item = bad;
};

#pragma pack(push, 1)
struct Primitive {
	union {
		bool b;
		float fdata;
		int idata;
	};

	PrimitiveType type = bad;
};
#pragma pack(pop)

struct Swizzle {
	index_t src = -1;
	SwizzleCode code;
};

struct Operation {
	index_t args = -1;
	index_t type = -1;
	OperationCode code;

};

#pragma pack(push, 1)
struct Intrinsic {
	index_t args = -1;
	index_t type = -1;
	// TODO: index into a table of names
	const char *name = nullptr;
};
#pragma pack(pop)

struct List {
	index_t item = -1;
	index_t next = -1;
};

struct Construct {
	index_t type = -1;
	index_t args = -1;
};

struct Call {
	index_t cid = -1;
	index_t args = -1;
	index_t type = -1;
};

struct Store {
	index_t dst = -1;
	index_t src = -1;
};

struct Load {
	index_t src = -1;
	index_t idx = -1;
};

struct Branch {
	index_t cond = -1;
	index_t failto = -1;
};

struct While {
	index_t cond = -1;
	index_t failto = -1;
};

struct Returns {
	index_t type = -1;
	index_t args = -1;
};

struct End {};

// Atom instructions
using __atom_base = wrapped::variant <
	Global, TypeField, Primitive,
	Swizzle, Operation, Construct, Call,
	Intrinsic, List, Store, Load,
	Branch, While, Returns, End
>;

struct alignas(4) Atom : __atom_base {
	using __atom_base::__atom_base;
};

// TODO: move to guarantees.hpp
static_assert(sizeof(Global)    == 6);
static_assert(sizeof(TypeField) == 6);
static_assert(sizeof(Primitive) == 5);
static_assert(sizeof(Swizzle)   == 4);
static_assert(sizeof(Operation) == 6);
static_assert(sizeof(Intrinsic) == 12);
static_assert(sizeof(List)      == 4);
static_assert(sizeof(Construct) == 4);
static_assert(sizeof(Call)      == 6);
static_assert(sizeof(Store)     == 4);
static_assert(sizeof(Load)      == 4);
static_assert(sizeof(Branch)    == 4);
static_assert(sizeof(While)     == 4);
static_assert(sizeof(Returns)   == 4);
static_assert(sizeof(End)       == 1);
static_assert(sizeof(Atom)      == 16);

// Dispatcher types (vd = variadic dispatcher)
std::string type_name(const Atom *const,
		      const wrapped::hash_table <int, std::string> &,
		      int, int);

// Printing instructions for debugging
void dump_ir_operation(const Atom &);

// Reindexing integer elements during compaction
void reindex_ir_operation(const wrapped::reindex <index_t> &, Atom &);

// Synthesizing GLSL source code
std::string synthesize_glsl_body(const Atom *const,
		                 const wrapped::hash_table <int, std::string> &,
		                 const std::unordered_set <index_t> &,
				 size_t);

} // namespace jvl::thunder
