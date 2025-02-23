#include "thunder/qualified_type.hpp"
#include "thunder/enumerations.hpp"

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

std::size_t NilType::hash() const
{
        return 0;
}

// Primitives and user-defined structures
bool PlainDataType::operator==(const PlainDataType &other) const
{
        if (index() != other.index())
                return false;

        if (auto p = get <PrimitiveType> ())
                return *p == other.as <PrimitiveType> ();

        return as <Index> () == other.as <Index> ();
}

std::string PlainDataType::to_string() const
{
        if (auto p = get <PrimitiveType> ())
                return tbl_primitive_types[*p];

        return fmt::format("concrete(%{})", as <Index> ());
}

std::size_t PlainDataType::hash() const
{
        if (is <PrimitiveType> ())
                return as <PrimitiveType> ();

        return std::size_t(as <Index> ()) << 32;
}

// Struct fields
StructFieldType::StructFieldType(PlainDataType ut_, Index next_)
                : PlainDataType(ut_), next(next_) {}

bool StructFieldType::operator==(const StructFieldType &other) const
{
        return PlainDataType::operator==(other)
                && (next == other.next);
}

std::string StructFieldType::to_string() const
{
        return fmt::format("{}; %{}", PlainDataType::to_string(), next);
}

PlainDataType StructFieldType::base() const
{
        return PlainDataType(*this);
}

std::size_t StructFieldType::hash() const
{
        return PlainDataType::hash() ^ (std::size_t(next) << 16);
}

// Array of unqualified types
ArrayType::ArrayType(PlainDataType ut_, Index size_)
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

std::size_t ArrayType::hash() const
{
        return PlainDataType::hash() ^ (std::size_t(size) << 16);
}

// Image types
ImageType::ImageType(PlainDataType result, Index dimension_)
                : PlainDataType(result), dimension(dimension_) {}

bool ImageType::operator==(const ImageType &other) const
{
        return PlainDataType::operator==(other)
                && (dimension == other.dimension);
}

std::string ImageType::to_string() const
{
        switch (as <PrimitiveType> ()) {
        case ivec4:
                return fmt::format("iimage{}D", dimension);
        case uvec4:
                return fmt::format("uimage{}D", dimension);
        case vec4:
                return fmt::format("image{}D", dimension);
        default:
                break;
        }

        return fmt::format("<?>image{}D", dimension);
}

std::size_t ImageType::hash() const
{
        return PlainDataType::hash() ^ (std::size_t(dimension) << 16);
}

// Sampler types
SamplerType::SamplerType(PlainDataType result, Index dimension_)
                : PlainDataType(result), dimension(dimension_) {}

bool SamplerType::operator==(const SamplerType &other) const
{
        return PlainDataType::operator==(other)
                && (dimension == other.dimension);
}

std::string SamplerType::to_string() const
{
        switch (as <PrimitiveType> ()) {
        case ivec4:
                return fmt::format("isampler{}D", dimension);
        case uvec4:
                return fmt::format("usampler{}D", dimension);
        case vec4:
                return fmt::format("sampler{}D", dimension);
        default:
                break;
        }

        return fmt::format("<?>sampler{}D", dimension);
}

std::size_t SamplerType::hash() const
{
        return PlainDataType::hash() ^ (std::size_t(dimension) << 16);
}

// Intrinsic types
IntrinsicType::IntrinsicType(QualifierKind kind_) : kind(kind_) {}

bool IntrinsicType::operator==(const IntrinsicType &other) const
{
        return kind == other.kind;
}

std::string IntrinsicType::to_string() const
{
        return tbl_qualifier_kind[kind];
}

std::size_t IntrinsicType::hash() const
{
        return kind;
}

// Buffer reference type
BufferReferenceType::BufferReferenceType(PlainDataType p, Index u)
                : PlainDataType(p), unique(u) {}

bool BufferReferenceType::operator==(const BufferReferenceType &other) const
{
	return PlainDataType::operator==(other)
                && unique == other.unique;
}

std::string BufferReferenceType::to_string() const
{
	return fmt::format("buffer({}; #{})", PlainDataType::to_string(), unique);
}

std::size_t BufferReferenceType::hash() const
{
	return PlainDataType::hash() ^ unique;
}

// Parameter IN type
InArgType::InArgType(PlainDataType p) : PlainDataType(p) {}

bool InArgType::operator==(const InArgType &other) const
{
	return PlainDataType::operator==(other);
}

std::string InArgType::to_string() const
{
	return fmt::format("in({})", PlainDataType::to_string());
}

std::size_t InArgType::hash() const
{
	return PlainDataType::hash();
}

// Parameter OUT type
OutArgType::OutArgType(PlainDataType p) : PlainDataType(p) {}

bool OutArgType::operator==(const OutArgType &other) const
{
	return PlainDataType::operator==(other);
}

std::string OutArgType::to_string() const
{
	return fmt::format("out({})", PlainDataType::to_string());
}

std::size_t OutArgType::hash() const
{
	return PlainDataType::hash();
}

// Parameter INOUT type
InOutArgType::InOutArgType(PlainDataType p) : PlainDataType(p) {}

bool InOutArgType::operator==(const InOutArgType &other) const
{
	return PlainDataType::operator==(other);
}

std::string InOutArgType::to_string() const
{
	return fmt::format("inout({})", PlainDataType::to_string());
}

std::size_t InOutArgType::hash() const
{
	return PlainDataType::hash();
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

bool QualifiedType::is_primitive() const
{
        auto pd = get <PlainDataType> ();
        return pd && pd->is <PrimitiveType> ();
}

bool QualifiedType::is_concrete() const
{
        auto pd = get <PlainDataType> ();
        return pd && pd->is <Index> ();
}

PlainDataType QualifiedType::remove_qualifiers() const
{
        switch (index()) {
        variant_case(QualifiedType, PlainDataType):
                return as <PlainDataType> ();
        variant_case(QualifiedType, StructFieldType):
                return as <StructFieldType> ().base();
        variant_case(QualifiedType, BufferReferenceType):
                return as <BufferReferenceType> ();
        default:
                break;
        }
        
        JVL_ABORT("failed to remove qualifiers from {}", to_string());
}

QualifiedType QualifiedType::primitive(PrimitiveType primitive)
{
        return PlainDataType(primitive);
}

QualifiedType QualifiedType::concrete(Index concrete)
{
        return PlainDataType(concrete);
}

QualifiedType QualifiedType::array(const QualifiedType &element, Index size)
{
        // Must be an unqualified type
        JVL_ASSERT(element.is <PlainDataType> (),
                "array element must be an unqualified type, got {} instead",
                element.to_string());

        return ArrayType(element.as <PlainDataType> (), size);
}

QualifiedType QualifiedType::image(PrimitiveType result, Index dimension)
{
        return ImageType(result, dimension);
}

QualifiedType QualifiedType::sampler(PrimitiveType result, Index dimension)
{
        return SamplerType(result, dimension);
}

QualifiedType QualifiedType::intrinsic(QualifierKind kind)
{
        return IntrinsicType(kind);
}

} // namespace jvl::thunder
