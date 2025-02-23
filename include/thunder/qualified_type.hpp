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
using plain_data_type_base = bestd::variant <PrimitiveType, Index>;

struct PlainDataType : public plain_data_type_base {
	using plain_data_type_base::plain_data_type_base;

	bool operator==(const PlainDataType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Plain data type which is a struct field
struct StructFieldType : PlainDataType {
	using PlainDataType::PlainDataType;

	Index next;

	StructFieldType(PlainDataType, Index);

	PlainDataType base() const;

	bool operator==(const StructFieldType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Array of unqualified types
struct ArrayType : public PlainDataType {
	Index size;

	ArrayType(PlainDataType, Index);

	PlainDataType base() const;

	bool operator==(const ArrayType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Image type, parameterized by underlying type and dimension
struct ImageType : public PlainDataType {
	Index dimension;

	ImageType(PlainDataType, Index);

	bool operator==(const ImageType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Sampler type, parameterized by underlying type and dimension
struct SamplerType : public PlainDataType {
	Index dimension;

	SamplerType(PlainDataType, Index);

	bool operator==(const SamplerType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// General intrinsic types
struct IntrinsicType {
	thunder::QualifierKind kind;

	IntrinsicType(thunder::QualifierKind);
	
	bool operator==(const IntrinsicType &) const;
	std::string to_string() const;
	std::size_t hash() const;
};

// Buffere reference types
// TODO: refactor; this includes buffers and buffer references...
struct BufferReferenceType : public PlainDataType {
	Index unique;

	BufferReferenceType(PlainDataType, Index);

	bool operator==(const BufferReferenceType &) const;
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
	IntrinsicType,
	BufferReferenceType,
	InArgType,
	OutArgType,
	InOutArgType
>;

struct QualifiedType : qualified_type_base {
	using qualified_type_base::qualified_type_base;

	operator bool() const;
	bool operator==(const QualifiedType &) const;
	std::string to_string() const;

	bool is_primitive() const;
	bool is_concrete() const;

	PlainDataType remove_qualifiers() const;

	// TODO: remove these...
	static QualifiedType primitive(PrimitiveType);
	static QualifiedType concrete(Index);
	static QualifiedType array(const QualifiedType &, Index);
	static QualifiedType image(PrimitiveType, Index);
	static QualifiedType sampler(PrimitiveType, Index);
	static QualifiedType intrinsic(QualifierKind);
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