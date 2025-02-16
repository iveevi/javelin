#include "core/logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/properties.hpp"
#include "thunder/qualified_type.hpp"
#include "thunder/overload.hpp"

namespace jvl::thunder {

MODULE(classify-atoms);

QualifiedType Buffer::classify(index_t i)
{
	auto transfer_decorations = [&](index_t src) {
		if (!used_decorations.contains(src))
			return;
		
		used_decorations[i] = used_decorations[src];
	};

	// Caching
	if (types[i])
		return types[i];

	auto &atom = atoms[i];

	switch (atom.index()) {

	case Atom::type_index <TypeInformation> ():
	{
		auto &type_field = atom.as <TypeInformation> ();

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

		return QualifiedType::nil();
	}

	case Atom::type_index <Primitive> ():
		return QualifiedType::primitive(atom.as <Primitive> ().type);

	case Atom::type_index <Qualifier> ():
	{
		auto &qualifier = atom.as <Qualifier> ();

		// Handling images and samplers
		if (image_kind(qualifier.kind)) {
			return QualifiedType::image(image_result(qualifier.kind),
						    image_dimension(qualifier.kind));
		}
		
		if (sampler_kind(qualifier.kind)) {
			return QualifiedType::sampler(sampler_result(qualifier.kind),
						      sampler_dimension(qualifier.kind));
		}

		// Ensure valid underlying type; OK to be invalid for images and samplers
		JVL_ASSERT(qualifier.underlying >= 0
			&& qualifier.underlying < (index_t) atoms.size(),
			"qualifier with invalid underlying reference: {}", qualifier);

		QualifiedType decl = classify(qualifier.underlying);

		// Extended qualifiers
		if (qualifier.kind == write_only || qualifier.kind == read_only) {
			// TODO: quick sanity check; only images and buffers allowed
			// JVL_ASSERT(image_kind())
			return decl;
		}

		// Handling arrays
		if (qualifier.kind == arrays) {
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
		transfer_decorations(qualifier.underlying);

		return base;
	}

	case Atom::type_index <Construct> ():
	{
		auto &constructor = atom.as <Construct> ();

		// Always transfer name hints
		transfer_decorations(constructor.type);

		QualifiedType qt = classify(constructor.type);
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

		// TODO: binary ops, no vector
		auto result = lookup_operation_overload(operation.code, qts);
		if (!result)
			dump();

		JVL_ASSERT(result, "failed to find overload for operation: {} (@{})", atom, i);

		return result;
	}

	case Atom::type_index <Intrinsic> ():
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		auto args = expand_list_types(intrinsic.args);
		auto result = lookup_intrinsic_overload(intrinsic.opn, args);
		if (!result)
			dump();
		JVL_ASSERT(result, "failed to find overload for intrinsic: {} (@{})", atom, i);
		return result;
	}

	case Atom::type_index <Swizzle> ():
	{
		auto &swz = atom.as <Swizzle> ();
		QualifiedType decl = classify(swz.src);
		PlainDataType plain = decl.remove_qualifiers();
		JVL_ASSERT(plain.is <PrimitiveType> (),
			"swizzle should only operate on vector types, "
			"but operand is {} with type {}",
			atoms[swz.src], decl.to_string());

		PrimitiveType swizzled = swizzle_type_of(plain.as <PrimitiveType> (), swz.code);

		return QualifiedType::primitive(swizzled);
	}

	case Atom::type_index <Load> ():
	{
		auto &load = atom.as <Load> ();

		QualifiedType qt = classify(load.src);
		if (load.idx == -1)
			return qt;

		switch (qt.index()) {

		case QualifiedType::type_index <PlainDataType> ():
		{
			auto &pd = qt.as <PlainDataType> ();
			JVL_ASSERT(pd.is <index_t> (), "cannot load field/element from primitive type: {}", qt);

			index_t concrete = pd.as <index_t> ();
			index_t left = load.idx;
			while (left--) {
				JVL_ASSERT(concrete != -1, "load attempting to access out of bounds field");
				auto &atom = atoms[concrete];
				JVL_ASSERT(atom.is <TypeInformation> (), "expected type information, instead got: {}", atom);
				auto &ti = atom.as <TypeInformation> ();
				concrete = ti.next;
			}

			return types[concrete].remove_qualifiers();
		}

		}

		dump();

		JVL_ABORT("unfinished implementation for load: {}", atom);
	}

	case Atom::type_index <ArrayAccess> ():
	{
		auto &access = atom.as <ArrayAccess> ();
		QualifiedType qt = classify(access.src);

		auto pd = qt.get <PlainDataType> ();
		while (pd) {
			if (pd->is <index_t> ()) {
				qt = classify(pd->as <index_t> ());
				pd = qt.get <PlainDataType> ();
			} else {
				dump();

				JVL_ABORT("unexpected path, is the type an array?");
			}
		}

		if (!qt.is <ArrayType> ()) {
			dump();

			JVL_ABORT("array accesses must operate on array "
				"types, but source is of type {}", qt);
		}

		// Check for possible name hints
		auto base = qt.as <ArrayType> ().base();

		auto concrete = base.get <index_t> ();
		if (concrete && used_decorations.contains(*concrete))
			transfer_decorations(*concrete);

		return base;
	}

	case Atom::type_index <Returns> ():
	{
		auto &returns = atom.as <Returns> ();
		return types[returns.value];
	}

	case Atom::type_index <List> ():
	case Atom::type_index <Store> ():
	case Atom::type_index <Branch> ():
		return QualifiedType::nil();

	default:
		break;
	}

	JVL_ABORT("failed to classify instruction: {}", atom);
}

} // namespace jvl::thunder
