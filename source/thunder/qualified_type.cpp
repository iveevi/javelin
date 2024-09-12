#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

MODULE(qualified-type);

// Nil
bool NilType::operator==(const NilType &) const
{
        return true;
}

std::string NilType::to_string() const
{
        return "nil";
}

// Primitives and user-defined structures
bool PlainDataType::operator==(const PlainDataType &other) const
{
        if (index() != other.index())
                return false;

        if (auto p = get <PrimitiveType> ())
                return *p == other.as <PrimitiveType> ();

        return as <index_t> () == other.as <index_t> ();
}

std::string PlainDataType::to_string() const
{
        if (auto p = get <PrimitiveType> ())
                return tbl_primitive_types[*p];

        return fmt::format("concrete(%{})", as <index_t> ());
}

// Struct fields
StructFieldType::StructFieldType(PlainDataType ut_, index_t next_)
        : PlainDataType(ut_), next(next_) {}

bool StructFieldType::operator==(const StructFieldType &other) const
{
        return PlainDataType::operator==(other)
                && (next == other.next);
}

std::string StructFieldType::to_string() const
{
        return fmt::format("{:15} next: %{}", PlainDataType::to_string(), next);
}

PlainDataType StructFieldType::base() const
{
        return PlainDataType(*this);
}

// Array of unqualified types
ArrayType::ArrayType(PlainDataType ut_, index_t size_)
        : PlainDataType(ut_), size(size_) {}

bool ArrayType::operator==(const ArrayType &other) const
{
        return PlainDataType::operator==(other)
                && (size == other.size);
}

std::string ArrayType::to_string() const
{
        return fmt::format("{}[{}]", PlainDataType::to_string(), size);
}

PlainDataType ArrayType::base() const
{
        return PlainDataType(*this);
}

// Full qualified type
QualifiedType::operator bool() const
{
        return !is <NilType> ();
}

template <typename A, typename B>
static constexpr bool compare(const A &a, const B &b)
{
	return false;
}

template <typename A>
static bool compare(const A &a, const A &b)
{
	return a == b;
}

bool QualifiedType::operator==(const QualifiedType &other) const
{
	auto ftn = [](auto a, auto b) { return compare(a, b); };
	return std::visit(ftn, *this, other);
}

std::string QualifiedType::to_string() const
{
        auto ftn = [](const auto &x) -> std::string { return x.to_string(); };
        return std::visit(ftn, *this);
}

PlainDataType QualifiedType::remove_qualifiers() const
{
        switch (index()) {
        case type_index <PlainDataType> ():
                return as <PlainDataType> ();
        case type_index <StructFieldType> ():
                return as <StructFieldType> ().base();
        default:
                break;
        }
        
        JVL_ABORT("failed to remove qualifiers from {}", to_string());
}

QualifiedType QualifiedType::primitive(PrimitiveType primitive)
{
        return PlainDataType(primitive);
}

QualifiedType QualifiedType::concrete(index_t concrete)
{
        return PlainDataType(concrete);
}

QualifiedType QualifiedType::array(const QualifiedType &element, index_t size)
{
        // Must be an unqualified type
        JVL_ASSERT(element.is <PlainDataType> (),
                "array element must be an unqualified type, got {} instead",
                element.to_string());

        return ArrayType(element.as <PlainDataType> (), size);
}

QualifiedType QualifiedType::nil()
{
        return NilType();
}

} // namespace jvl::thunder