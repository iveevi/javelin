#pragma once

#include <string>
#include <filesystem>

#include <bestd/variant.hpp>

#include <fmt/format.h>

#include "../enumerations.hpp"
#include "memory.hpp"

namespace jvl::thunder::mir {

//////////////////////////////////////////
// Molecule intermediate representation //
//////////////////////////////////////////

// Value aliases
using Int = int;
using Float = float;
using Bool = bool;

// TODO: override ptr char to fit packed
using String = std::string;

// Top level molecule
struct Molecule;

// Aliases for memory management
template <typename T>
using Ref = Ptr <T, Molecule>;

template <typename T>
using Seq = List <T, Molecule>;

// Type information
struct Type;

struct Field : bestd::variant <Ref <Type>, PrimitiveType> {
	using bestd::variant <Ref <Type>, PrimitiveType>::variant;
};

struct Type {
	Seq <Field> fields;
	Seq <QualifierKind> qualifiers;
};

struct Primitive : bestd::variant <Int, Float, Bool, String> {
	using bestd::variant <Int, Float, Bool, String>::variant;
};

struct Storage {
	Ref <Type> type;
	Ref <Molecule> value;
};

struct Operation {
	Ref <Molecule> a;
	Ref <Molecule> b;
	thunder::OperationCode code;
};

// TODO: use something else for intrinsic...
struct Intrinsic {
	Seq <Ref <Molecule>> args;
	thunder::IntrinsicOperation code;
};

struct Construct {
	Ref <Type> type;
	Seq <Ref <Molecule>> args;
	thunder::ConstructorMode mode;
};

struct Call {
	Index callee;
	Seq <Ref <Molecule>> args;
};

struct Store {
	Ref <Molecule> dst;
	Ref <Molecule> src;
};

struct Load {
	Ref <Molecule> src;
	Int idx;
};

struct Indexing {
	Ref <Molecule> src;
	Ref <Molecule> idx;
};

struct Block {
	Ref <Block> parent;
	Seq <Ref <Molecule>> body;

	void graphviz(const std::filesystem::path &) const;
};

std::string format_as(const Block &);

struct Branch {
	Ref <Molecule> condition;
	Ref <Block> then;
	Ref <Block> otherwise;
};

struct Phi {
	Seq <Ref <Molecule>> values;
	Seq <Ref <Block>> blocks;
};

struct Return {
	Ref <Molecule> value;
};

using molecule_base = bestd::variant <
	// Primary instructions
	Type,
	Primitive,
	Operation,
	Intrinsic,
	Construct,
	Call,
	Store,
	Load,
	Indexing,
	Block,
	Branch,
	Phi,
	Return,
	
	// Misecellaneous; for memory arena
	Ref <Molecule>,
	Field,
	QualifierKind
>;

struct Molecule : molecule_base {
	using molecule_base::molecule_base;
};

} // namespace jvl::thunder::mir