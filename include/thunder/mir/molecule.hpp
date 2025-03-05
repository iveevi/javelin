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

struct Molecule;

template <typename T>
using Ref = Ptr <T, Molecule>;

template <typename T>
using Seq = List <T, Molecule>;

struct Type;
struct Field : bestd::variant <Ref <Type>, thunder::PrimitiveType> {
	using bestd::variant <Ref <Type>, thunder::PrimitiveType>::variant;

	std::string to_string() const;
};

// TODO: use an arena allocator for this
struct Type {
	Seq <Field> fields;
	Seq <thunder::QualifierKind> qualifiers;

	std::string to_string() const {
		std::string result;

		for (auto &field : fields)
			result += fmt::format("{} ", field.to_string());

		if (qualifiers.count > 0)
			result += ": ";

		for (auto &qualifier : qualifiers)
			result += fmt::format("{} ", thunder::tbl_qualifier_kind[qualifier]);

		return result;
	}
};

std::string Field::to_string() const
{
	if (auto type = this->get <Ref <Type>> ())
		return type.value()->to_string();
	else
		return thunder::tbl_primitive_types[this->as <thunder::PrimitiveType> ()];
}

struct Primitive : bestd::variant <Int, Float, Bool, String> {
	using bestd::variant <Int, Float, Bool, String>::variant;

	std::string to_string() const {
		switch (this->index()) {
		variant_case(Primitive, Int):
			return fmt::format("i32 {}", this->as <Int> ());
		variant_case(Primitive, Float):
			return fmt::format("f32 {}", this->as <Float> ());
		variant_case(Primitive, Bool):
			return fmt::format("bool {}", this->as <Bool> ());
		variant_case(Primitive, String):
			return fmt::format("string \"{}\"", this->as <String> ());
		}

		return "not implemented";
	}
};

struct Operation {
	Ref <Molecule> a;
	Ref <Molecule> b;
	thunder::OperationCode code;

	std::string to_string() const {
		if (b) {
			return fmt::format("{} {} {}",
				thunder::tbl_operation_code[code],
				a.index.value,
				b.index.value);
		} else {
			return fmt::format("{} {}",
				thunder::tbl_operation_code[code],
				a.index.value);
		}
	}
};

// TODO: use something else for intrinsic...
struct Intrinsic {
	Seq <Ref <Molecule>> args;
	thunder::IntrinsicOperation opn;

	std::string to_string() const {
		std::string result;

		result += fmt::format("{} ", thunder::tbl_intrinsic_operation[opn]);

		for (auto &arg : args)
			result += fmt::format("{} ", arg.index.value);

		return result;
	}
};

struct Construct {
	Ref <Type> type;
	Seq <Ref <Molecule>> args;
	thunder::ConstructorMode mode;

	std::string to_string() const {
		std::string result;

		result += fmt::format("{} new ", type.index.value);
		for (auto &arg : args)
			result += fmt::format("{} ", arg.index.value);

		result += fmt::format(": {}", thunder::tbl_constructor_mode[mode]);

		return result;
	}
};

struct Call {
	Index callee;
	Seq <Ref <Molecule>> args;
};

struct Store {
	Ref <Molecule> dst;
	Ref <Molecule> src;

	std::string to_string() const {
		return fmt::format("store {} {}", dst.index.value, src.index.value);
	}
};

struct Load {
	Ref <Molecule> src;
	Int idx;

	std::string to_string() const {
		return fmt::format("load {} {}", src.index.value, idx);
	}
};

struct Indexing {
	Ref <Molecule> src;
	Ref <Molecule> idx;

	std::string to_string() const {
		return fmt::format("index {} {}", src.index.value, idx.index.value);
	}
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

	std::string to_string() const {
		return fmt::format("return {}", value.index.value);
	}
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