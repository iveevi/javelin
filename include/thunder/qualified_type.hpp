#pragma once

#include "atom.hpp"
#include "enumerations.hpp"

namespace jvl::thunder {

// Not a valid type
struct NilType {
	bool operator==(const NilType &) const;
	std::string to_string() const;
};

// Either a primitive type or a user-defined structure
using plain_data_type = wrapped::variant <PrimitiveType, index_t>;

struct PlainDataType : public plain_data_type {
	using plain_data_type::plain_data_type;

	bool operator==(const PlainDataType &) const;
	std::string to_string() const;
};

// Plain data type which is a struct field
struct StructFieldType : PlainDataType {
	using PlainDataType::PlainDataType;

	index_t next;

	StructFieldType(PlainDataType, index_t);
	bool operator==(const StructFieldType &) const;
	std::string to_string() const;
	PlainDataType base() const;
};

// Array of unqualified types
struct ArrayType : public PlainDataType {
	index_t size;

	ArrayType(PlainDataType, index_t);
	bool operator==(const ArrayType &) const;
	std::string to_string() const;
	PlainDataType base() const;
};

// TODO: inout
// TODO: out
// TODO: storage types

// Full qualified type
using qualified_type_base = wrapped::variant <
	NilType,
	PlainDataType,
	StructFieldType,
	ArrayType
>;

struct QualifiedType : qualified_type_base {
	using qualified_type_base::qualified_type_base;

	operator bool() const;
	bool operator==(const QualifiedType &) const;
	std::string to_string() const;
	PlainDataType remove_qualifiers() const;

	static QualifiedType primitive(PrimitiveType);
	static QualifiedType concrete(index_t);
	static QualifiedType array(const QualifiedType &, index_t);
	static QualifiedType nil();
};

// Support for direct printing through fmt
inline auto format_as(const QualifiedType &qt)
{
	return qt.to_string();
}

} // namespace jvl::thunder