#pragma once

#include <string>

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

struct Field : bestd::variant <Ref <Type>, thunder::PrimitiveType> {
	using bestd::variant <Ref <Type>, thunder::PrimitiveType>::variant;

	std::string to_string() const;
};

struct Type {
	Seq <Field> fields;
	Seq <thunder::QualifierKind> qualifiers;

	std::string to_string() const;
};

struct Primitive : bestd::variant <Int, Float, Bool, String> {
	using bestd::variant <Int, Float, Bool, String>::variant;

	std::string to_string() const;
};

struct Operation {
	Ref <Molecule> a;
	Ref <Molecule> b;
	thunder::OperationCode code;

	std::string to_string() const;
};

// TODO: use something else for intrinsic...
struct Intrinsic {
	Seq <Ref <Molecule>> args;
	thunder::IntrinsicOperation opn;

	std::string to_string() const;
};

struct Construct {
	Ref <Type> type;
	Seq <Ref <Molecule>> args;
	thunder::ConstructorMode mode;

	std::string to_string() const;
};

struct Call {
	Index callee;
	Seq <Ref <Molecule>> args;
};

struct Store {
	Ref <Molecule> dst;
	Ref <Molecule> src;

	std::string to_string() const;
};

struct Load {
	Ref <Molecule> src;
	Int idx;

	std::string to_string() const;
};

struct Indexing {
	Ref <Molecule> src;
	Ref <Molecule> idx;

	std::string to_string() const;
};

struct Block {
	Ref <Block> parent;
	Seq <Ref <Molecule>> body;

	std::string to_string() const;
};

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

	std::string to_string() const;
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
	thunder::QualifierKind
>;

template <typename T>
concept stringable = requires(T t) {
	{ t.to_string() } -> std::convertible_to <std::string>;
};

struct Molecule : molecule_base {
	using molecule_base::molecule_base;

	std::string to_string() const {
		auto ftn = [](auto &self) -> std::string {
			if constexpr (stringable <decltype(self)>)
				return self.to_string();
			else
				return "not implemented";
		};

		return std::visit(ftn, *this);
	}
};

} // namespace jvl::thunder::mir