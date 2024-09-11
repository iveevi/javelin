#pragma once

#include "atom.hpp"
#include "enumerations.hpp"

namespace jvl::thunder {

// Not a valid type
struct NilType {
	bool operator==(const NilType &) const {
		return true;
	}

	std::string to_string() const {
		return "nil";
	}
};

// Either a primitive type or a user-defined structure
using plain_data_type = wrapped::variant <PrimitiveType, index_t>;

struct PlainDataType : public plain_data_type {
	using plain_data_type::plain_data_type;

	bool operator==(const PlainDataType &other) const {
		if (index() != other.index())
			return false;

		if (auto p = get <PrimitiveType> ())
			return *p == other.as <PrimitiveType> ();

		return as <index_t> () == other.as <index_t> ();
	}

	std::string to_string() const {
		if (auto p = get <PrimitiveType> ())
			return tbl_primitive_types[*p];

		return fmt::format("concrete(%{})", as <index_t> ());
	}
};

// Array of unqualified types
struct ArrayType : public PlainDataType {
	index_t size;

	ArrayType(PlainDataType ut_, index_t size_)
		: PlainDataType(ut_), size(size_) {}

	bool operator==(const ArrayType &other) const {
		return PlainDataType::operator==(other)
			&& (size == other.size);
	}

	std::string to_string() const {
		return fmt::format("{}[{}]", PlainDataType::to_string(), size);
	}
};

// TODO: inout
// TODO: out
// TODO: storage types

// Full qualified type
using qualified_type_base = wrapped::variant <
	NilType,
	PlainDataType,
	ArrayType
>;

struct QualifiedType : qualified_type_base {
	using qualified_type_base::qualified_type_base;

	// TODO: source file;
	operator bool() const {
		return !is <NilType> ();
	}

	bool operator==(const QualifiedType &other) const {
		MODULE(qualified-type-comparison);

		if (index() != other.index())
			return true;

		switch (index()) {
		case type_index <NilType> ():
			return as <NilType> () == other.as <NilType> ();
		case type_index <PlainDataType> ():
			return as <PlainDataType> () == other.as <PlainDataType> ();
		case type_index <ArrayType> ():
			return as <ArrayType> () == other.as <ArrayType> ();
		default:
			break;
		}

		JVL_ABORT("unhandled case in qualified type: {}", to_string());
	}
	
	std::string to_string() const {
		auto ftn = [](const auto &x) -> std::string { return x.to_string(); };
		return std::visit(ftn, *this);
	}

	PlainDataType remove_qualifiers() const {
		MODULE(qualified-type-remove-qualifiers);

		switch (index()) {
		default:
			break;
		}
		
		JVL_ABORT("cannot remove qualifiers from {}", to_string());
	}

	static QualifiedType primitive(PrimitiveType primitive) {
		return PlainDataType(primitive);
	}
	
	static QualifiedType concrete(index_t concrete) {
		return PlainDataType(concrete);
	}

	static QualifiedType array(const QualifiedType &element, index_t size) {
		MODULE(qualified-type-array);

		// Must be an unqualified type
		JVL_ASSERT(element.is <PlainDataType> (),
			"array element must be an unqualified type, got {} instead",
			element.to_string());

		return ArrayType(element.as <PlainDataType> (), size);
	}

	static QualifiedType nil() {
		return NilType();
	}
};

// Support for direct printing through fmt
inline auto format_as(const QualifiedType &qt)
{
	return qt.to_string();
}

} // namespace jvl::thunder