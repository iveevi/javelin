#include "core/logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/properties.hpp"
#include "thunder/qualified_type.hpp"
#include "thunder/overload.hpp"

namespace jvl::thunder {

MODULE(classify-atoms);
	
void Buffer::transfer_decorations(Index dst, Index src)
{
	auto &used = decorations.used;
	if (used.contains(src))
		used[dst] = used[src];
}

QualifiedType Buffer::semalz_qualifier(const Qualifier &qualifier, Index i)
{
	// Handling images and samplers
	if (image_kind(qualifier.kind)) {
		return QualifiedType::image(image_result(qualifier.kind),
						image_dimension(qualifier.kind));
	}
	
	if (sampler_kind(qualifier.kind)) {
		return QualifiedType::sampler(sampler_result(qualifier.kind),
						sampler_dimension(qualifier.kind));
	}

	// General intrinsic types
	if (intrinsic_kind(qualifier.kind))
		return QualifiedType::intrinsic(qualifier.kind);

	// Ensure valid underlying type; OK to be invalid for images and samplers
	JVL_ASSERT(qualifier.underlying >= 0
		&& qualifier.underlying < (Index) atoms.size(),
		"qualifier with invalid underlying reference: {}", qualifier);

	QualifiedType decl = semalz(qualifier.underlying);

	// Extended qualifiers
	if (qualifier.kind == writeonly || qualifier.kind == readonly) {
		// TODO: quick sanity check; only images and buffers allowed
		// JVL_ASSERT(image_kind())
		return decl;
	}

	// Handling arrays
	if (qualifier.kind == arrays) {
		// Simplify if needed
		if (decl.is <StructFieldType> ())
			decl = QualifiedType::concrete(qualifier.underlying);

		return QualifiedType::array(decl, qualifier.numerical);
	}

	// Potentially primitive
	PlainDataType base = qualifier.underlying;
	if (auto pd = decl.get <PlainDataType> ()) {
		if (pd->is <PrimitiveType> ())
			base = pd->as <PrimitiveType> ();
	}

	// Buffers and references get wrapped
	if (qualifier.kind == buffer_reference || qualifier.kind == storage_buffer)
		return BufferReferenceType(base, qualifier.numerical);

	// Handling parameter types
	if (qualifier.kind == qualifier_in)
		return InArgType(base);
	if (qualifier.kind == qualifier_out)
		return OutArgType(base);
	if (qualifier.kind == qualifier_inout)
		return InOutArgType(base);

	// Simplify in the case of special parameter types
	if (decl.is <InArgType> ()
			|| decl.is <OutArgType> ()
			|| decl.is <InOutArgType> ())
		return decl;

	// Transfer name hints in the default case
	transfer_decorations(i, qualifier.underlying);

	return base;
}

QualifiedType Buffer::semalz_construct(const Construct &constructor, Index i)
{
	// Always transfer name hints
	transfer_decorations(i, constructor.type);

	QualifiedType qt = semalz(constructor.type);
	if (qt.is <PlainDataType> ())
		return qt;

	if (constructor.mode == transient) {
		// Reduce if its a parameter
		if (auto in = qt.get <InArgType> ())
			return static_cast <PlainDataType> (*in);
		if (auto out = qt.get <OutArgType> ())
			return static_cast <PlainDataType> (*out);
		if (auto inout = qt.get <InOutArgType> ())
			return static_cast <PlainDataType> (*inout);

		// Or other special kind of resource
		if (auto image = qt.get <ImageType> ())
			return static_cast <ImageType> (*image);
		if (auto sampler = qt.get <SamplerType> ())
			return static_cast <SamplerType> (*sampler);
	}

	return QualifiedType::concrete(constructor.type);
}

QualifiedType Buffer::semalz_load(const Load &load, Index i)
{
	QualifiedType qt = semalz(load.src);
	if (load.idx == -1)
		return qt;

	switch (qt.index()) {

	variant_case(QualifiedType, PlainDataType):
	{
		auto &pd = qt.as <PlainDataType> ();
		if (!pd.is <Index> ())
			dump();
		JVL_ASSERT(pd.is <Index> (), "cannot load from primitive type: {}", qt);

		Index concrete = pd.as <Index> ();

		auto &base = types[concrete];
		if (base.is <BufferReferenceType> ()) {
			auto &brt = base.as <BufferReferenceType> ();
			concrete = static_cast <PlainDataType> (brt).as <Index> ();
		}

		Index left = load.idx;
		while (left--) {
			JVL_BUFFER_DUMP_ON_ASSERT((concrete != -1),
				"load attempting to access "
				"out of bounds field");
			auto &atom = atoms[concrete];

			JVL_BUFFER_DUMP_ON_ASSERT(atom.is <TypeInformation> (),
				"expected type information, "
				"instead got:\n{}", atom);
			auto &ti = atom.as <TypeInformation> ();
			concrete = ti.next;
		}

		return types[concrete].remove_qualifiers();
	}

	default:
		break;
	}

	JVL_BUFFER_DUMP_AND_ABORT("unfinished implementation for load: {}", load);
}

QualifiedType Buffer::semalz_access(const ArrayAccess &access, Index i)
{
	QualifiedType qt = semalz(access.src);

	while (true) {
		if (auto pd = qt.get <PlainDataType> ()) {
			if (pd->is <Index> ())
				qt = semalz(pd->as <Index> ());
			else
				JVL_BUFFER_DUMP_AND_ABORT("unexpected path, is the base type an array?");
		} else if (auto sft = qt.get <StructFieldType> ()) {
			qt = sft->base();
		} else if (auto brt = qt.get <BufferReferenceType> ()) {
			qt = qt.remove_qualifiers();
		} else {
			break;
		}
	}

	if (!qt.is <ArrayType> ()) {
		JVL_BUFFER_DUMP_AND_ABORT("array accesses must operate "
			"on array types, but source is of type {}:\n{}", qt, atoms[i]);
	}

	// Check for possible name hints
	auto base = qt.as <ArrayType> ().base();

	auto concrete = base.get <Index> ();
	if (concrete && decorations.used.contains(*concrete))
		transfer_decorations(i, *concrete);

	return base;
}

QualifiedType Buffer::semalz(Index i)
{
	// Cached results
	if (types[i])
		return types[i];

	auto &atom = atoms[i];

	switch (atom.index()) {

	case Atom::type_index <TypeInformation> ():
	{
		auto &type_field = atom.as <TypeInformation> ();

		// Sometimes we explicitly want a nil type as a marker
		if (type_field.item == nil)
			return NilType();

		std::optional <PlainDataType> pd;
		if (type_field.item != bad)
			pd = PlainDataType(type_field.item);
		if (type_field.down != -1)
			pd = PlainDataType(type_field.down);

		if (pd) {
			if (type_field.next != -1)
				return StructFieldType(pd.value(), type_field.next);
			else
				return pd.value();
		}

		return NilType();
	}

	case Atom::type_index <Primitive> ():
		return QualifiedType::primitive(atom.as <Primitive> ().type);

	case Atom::type_index <Qualifier> ():
		return semalz_qualifier(atom.as <Qualifier> (), i);

	case Atom::type_index <Construct> ():
		return semalz_construct(atom.as <Construct> (), i);

	case Atom::type_index <Call> ():
	{
		auto &call = atom.as <Call> ();
		auto &qt = types[call.type];
		if (qt.is_primitive())
			return qt;

		return QualifiedType::concrete(call.type);
	}

	case Atom::type_index <Operation> ():
	{
		auto &operation = atom.as <Operation> ();

		JVL_ASSERT(operation.a >= 0 && (size_t) operation.a < pointer,
			"invalid operand argument in operation: {} (@{})", atom, i);

		if (operation.code != unary_negation && operation.code != bool_not) {
			JVL_ASSERT(operation.b >= 0 && (size_t) operation.b < pointer,
				"invalid operand argument in operation: {} (@{})", atom, i);
		}

		std::vector <QualifiedType> qts;
		qts.push_back(types[operation.a]);
		if (operation.b != -1)
			qts.push_back(types[operation.b]);

		auto result = lookup_operation_overload(operation.code, qts);
		
		JVL_ASSERT(result, "failed to find overload for operation: {} (@{})", atom, i);

		return result;
	}

	case Atom::type_index <Intrinsic> ():
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		auto args = expand_list_types(intrinsic.args);
		auto result = lookup_intrinsic_overload(intrinsic.opn, args);

		JVL_BUFFER_DUMP_ON_ASSERT(result, "failed to find overload for intrinsic: {} (@{})", atom, i);

		return result;
	}

	case Atom::type_index <Swizzle> ():
	{
		auto &swz = atom.as <Swizzle> ();
		
		QualifiedType decl = semalz(swz.src);
		QualifiedType plain = decl.remove_qualifiers();

		if (!plain.is_primitive())
			dump();

		JVL_ASSERT(plain.is_primitive(),
			"swizzle takes vector types, "
			"but operand has type {}:\n{}",
			decl.to_string(), atoms[swz.src]);

		PrimitiveType original = plain.as <PlainDataType> ().as <PrimitiveType> ();
		PrimitiveType swizzled = swizzle_type_of(original, swz.code);

		return PlainDataType(swizzled);
	}

	case Atom::type_index <Load> ():
		return semalz_load(atom.as <Load> (), i);

	case Atom::type_index <ArrayAccess> ():
		return semalz_access(atom.as <ArrayAccess> (), i);

	case Atom::type_index <Returns> ():
	{
		auto &returns = atom.as <Returns> ();
		return types[returns.value];
	}

	case Atom::type_index <List> ():
	case Atom::type_index <Store> ():
	case Atom::type_index <Branch> ():
		return NilType();

	default:
		break;
	}

	JVL_ABORT("failed to classify instruction: {}", atom);
}

} // namespace jvl::thunder
