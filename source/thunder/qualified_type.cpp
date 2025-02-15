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

        return as <index_t> () == other.as <index_t> ();
}

std::string PlainDataType::to_string() const
{
        if (auto p = get <PrimitiveType> ())
                return tbl_primitive_types[*p];

        return fmt::format("concrete(%{})", as <index_t> ());
}

std::size_t PlainDataType::hash() const
{
        if (is <PrimitiveType> ())
                return as <PrimitiveType> ();

        return std::size_t(as <index_t> ()) << 32;
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

std::size_t ArrayType::hash() const
{
        return PlainDataType::hash() ^ (std::size_t(size) << 16);
}

// Image types
ImageType::ImageType(PlainDataType result, index_t dimension_)
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
SamplerType::SamplerType(PlainDataType result, index_t dimension_)
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

QualifiedType QualifiedType::sampler(PrimitiveType result, index_t dimension)
{
        return SamplerType(result, dimension);
}

QualifiedType QualifiedType::image(PrimitiveType result, index_t dimension)
{
        return ImageType(result, dimension);
}

QualifiedType QualifiedType::nil()
{
        return NilType();
}

} // namespace jvl::thunder
