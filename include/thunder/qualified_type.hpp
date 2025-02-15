#pragma once

#include <bestd/variant.hpp>

#include "atom.hpp"
#include "enumerations.hpp"

namespace jvl::thunder {

// Not a valid type
struct NilType {
	bool operator==(const NilType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Either a primitive type or a user-defined structure
using plain_data_type_base = bestd::variant <PrimitiveType, index_t>;

struct PlainDataType : public plain_data_type_base {
	using plain_data_type_base::plain_data_type_base;

	bool operator==(const PlainDataType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Plain data type which is a struct field
struct StructFieldType : PlainDataType {
	using PlainDataType::PlainDataType;

	index_t next;

	StructFieldType(PlainDataType, index_t);

	PlainDataType base() const;

	bool operator==(const StructFieldType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Array of unqualified types
struct ArrayType : public PlainDataType {
	index_t size;

	ArrayType(PlainDataType, index_t);

	PlainDataType base() const;

	bool operator==(const ArrayType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Image type, parameterized by underlying type and dimension
struct ImageType : public PlainDataType {
	index_t dimension;

	ImageType(PlainDataType, index_t);

	bool operator==(const ImageType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Sampler type, parameterized by underlying type and dimension
struct SamplerType : public PlainDataType {
	index_t dimension;

	SamplerType(PlainDataType, index_t);

	bool operator==(const SamplerType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Parameter IN type
struct InArgType : public PlainDataType {
	InArgType(PlainDataType);

	bool operator==(const InArgType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Parameter OUT type
struct OutArgType : public PlainDataType {
	OutArgType(PlainDataType);

	bool operator==(const OutArgType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Parameter INOUT type
struct InOutArgType : public PlainDataType {
	InOutArgType(PlainDataType);

	bool operator==(const InOutArgType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Full qualified type
using qualified_type_base = bestd::variant <
	NilType,
	PlainDataType,
	StructFieldType,
	ArrayType,
	ImageType,
	SamplerType,
	InArgType,
	OutArgType,
	InOutArgType
>;

struct QualifiedType : qualified_type_base {
	using qualified_type_base::qualified_type_base;

	bool is_primitive() const {
		auto pd = get <PlainDataType> ();
		return pd && pd->is <PrimitiveType> ();
	}

	bool is_concrete() const {
		auto pd = get <PlainDataType> ();
		// TODO: alias ConcreteIndex = index_t
		return pd && pd->is <index_t> ();
	}

	PlainDataType remove_qualifiers() const;

	operator bool() const;
	bool operator==(const QualifiedType &) const;
	std::string to_string() const;

	static QualifiedType primitive(PrimitiveType);
	static QualifiedType concrete(index_t);
	static QualifiedType array(const QualifiedType &, index_t);
	static QualifiedType image(PrimitiveType, index_t);
	static QualifiedType sampler(PrimitiveType, index_t);
	static QualifiedType nil();
};

// Support for direct printing through fmt
inline auto format_as(const QualifiedType &qt)
{
	return qt.to_string();
}

} // namespace jvl::thunder

// Hashing
// TODO: replace with maps and comparison operator?
template <>
struct std::hash <jvl::thunder::QualifiedType> {
	std::size_t operator()(const jvl::thunder::QualifiedType &qt) const {
		auto ftn = [](auto x) { return x.hash(); };
		return std::visit(ftn, qt);
	}
};