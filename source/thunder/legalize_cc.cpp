#include "common/logging.hpp"

#include "ire/emitter.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/optimization.hpp"
#include "thunder/properties.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

MODULE(legalize-cc);

PrimitiveType primitive_type_of(const Buffer &buffer, Index i)
{
	auto &qt = buffer.types[i];

	switch (qt.index()) {

	case QualifiedType::type_index <NilType> ():
		return bad;

	case QualifiedType::type_index <PlainDataType> ():
	{
		auto pd = qt.as <PlainDataType> ();
		if (pd.is <PrimitiveType> ())
			return pd.as <PrimitiveType> ();

		// If it is a concrete type, then it was
		// unable to be resolved as a primitive
		return bad;
	}

	default:
		break;
	}

	JVL_ABORT("unhandled case in primitive_type_of: {}", qt);
}

void legalize_for_cc_operation_vector_overload(mapped_instruction_t &mapped,
					       OperationCode code,
					       Index a,
					       Index b,
					       PrimitiveType type_a,
					       PrimitiveType type_b)
{
	auto &em = ire::Emitter::active;

	// Reset and activate the scratch
	mapped.clear();
	em.push(mapped, false);

	// Assuming this is a binary instruction
	if (vector_type(type_a) && vector_type(type_b)) {
		PrimitiveType ctype = swizzle_type_of(type_a, SwizzleCode::x);
		size_t ccount = vector_component_count(type_a);

		JVL_ASSERT_PLAIN(ctype == swizzle_type_of(type_b, SwizzleCode::x));
		JVL_ASSERT_PLAIN(ccount == vector_component_count(type_b));

		std::vector <Index> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			Index ca = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(ca, 0b01);

			Index cb = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(cb, 0b01);

			components[i] = em.emit(Operation(ca, cb, code));
		}

		Index l = em.emit_list_chain(components);
		Index t = em.emit(TypeInformation(-1, -1, type_a));
		em.emit(Construct(t, l));

		// fmt::println("legalized code:");
		// mapped.transformed.dump();
		// JVL_ASSERT_PLAIN(false);
	} else if (vector_type(type_a) && !vector_type(type_b)) {
		JVL_ASSERT_PLAIN(type_b == swizzle_type_of(type_a, SwizzleCode::x));

		size_t ccount = vector_component_count(type_a);

		std::vector <Index> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			Index c = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(c, 0b01);

			components[i] = em.emit(Operation(c, b, code));
		}

		Index l = em.emit_list_chain(components);
		Index t = em.emit(TypeInformation(-1, -1, type_a));
		em.emit(Construct(t, l));
	} else if (!vector_type(type_a) && vector_type(type_b)) {
		JVL_ASSERT_PLAIN(type_a == swizzle_type_of(type_b, SwizzleCode::x));

		size_t ccount = vector_component_count(type_b);

		std::vector <Index> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			Index c = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(c, 0b01);

			components[i] = em.emit(Operation(a, c, code));
		}

		Index l = em.emit_list_chain(components);
		Index t = em.emit(TypeInformation(-1, -1, type_b));
		em.emit(Construct(t, l));
	} else {
		// TODO: legalize by converting to higher type
		JVL_ASSERT_PLAIN(false);
	}

	em.pop();
}

bool legalize_for_cc_vector_constructor(mapped_instruction_t &mapped,
					PrimitiveType type_to_construct,
					const std::vector <Index> &args,
					const std::vector <PrimitiveType> &types)
{
	bool transformed = false;

	// For now assume only one argument when constructing heterogenously
	JVL_ASSERT(args.size() == 1, "vector constructor legalization currently requires exactly one argument");
	
	auto &em = ire::Emitter::active;
	
	// Reset and activate the scratch
	mapped.clear();
	em.push(mapped, false);

	PrimitiveType type_a = types[0];
	if (type_a == type_to_construct) {
		size_t ccount = vector_component_count(type_to_construct);

		std::vector <Index> components;
		for (size_t i = 0; i < ccount; i++) {
			Index c = em.emit(Swizzle(args[0], (SwizzleCode) i));
			mapped.track(c, 0b01);
			components.push_back(c);
		}

		Index l = em.emit_list_chain(components);
		Index t = em.emit(TypeInformation(-1, -1, type_to_construct));
		em.emit_construct(t, l, normal);

		transformed = true;
	} else {
		JVL_ABORT("unsupported argument {} when legalizing constructor for {}",
			tbl_primitive_types[type_a],
			tbl_primitive_types[type_to_construct]);
	}

	em.pop();

	return transformed;
}

bool legalize_for_cc_intrinsic(mapped_instruction_t &mapped,
			       IntrinsicOperation opn,
			       const std::vector <Index> &args,
			       const std::vector <PrimitiveType> &types)
{
	bool transformed = false;

	// Intrinsics which can be successfully legalized
	static const std::set <IntrinsicOperation> legalizable {
		clamp,
		min, max,
		dot,
		sin, cos, tan,
		asin, acos, atan,
		pow
	};

	JVL_ASSERT(legalizable.contains(opn),
		"intrinsic operation ${} cannot be "
		"legalized for CC targets",
		tbl_intrinsic_operation[opn]);
	
	auto &em = ire::Emitter::active;

	// Special cases are handled explicitly
	if (opn == dot) {
		PrimitiveType type_a = types[0];
		PrimitiveType type_b = types[1];

		JVL_ASSERT_PLAIN(vector_type(type_a) && vector_type(type_b));
		JVL_ASSERT_PLAIN(swizzle_type_of(type_a, SwizzleCode::x) == swizzle_type_of(type_b, SwizzleCode::x));
		JVL_ASSERT_PLAIN(vector_component_count(type_a) == vector_component_count(type_b));

		// Reset and activate the scratch
		mapped.clear();
		em.push(mapped, false);

		size_t ccount = vector_component_count(type_a);

		Index a = args[0];
		Index b = args[1];
		
		std::vector <Index> products(ccount);
		for (size_t i = 0; i < ccount; i++) {
			Index ca = em.emit_swizzle(a, (SwizzleCode) i);
			mapped.track(ca, 0b01);

			Index cb = em.emit_swizzle(b, (SwizzleCode) i);
			mapped.track(cb, 0b01);

			products[i] = em.emit_operation(ca, cb, multiplication);
		}

		Index top = products[0];
		for (size_t i = 0; i < ccount - 1; i++)
			top = em.emit_operation(top, products[i + 1], addition);

		em.pop();

		transformed = true;
	}

	return transformed;
}

// TODO: legalization context...
void legalize_for_cc(Buffer &buffer)
{
	JVL_STAGE();

	auto &em = ire::Emitter::active;
	auto &atoms = buffer.atoms;

	std::vector <mapped_instruction_t> mapped(buffer.pointer);

	for (size_t i = 0; i < mapped.size(); i++) {
		auto &atom = atoms[i];

		// Default population of scratches is preservation
		em.push(mapped[i], false);
		em.emit(atom);
		em.pop();
	}

	for (size_t i = 0; i < mapped.size(); i++) {
		bool transformed = false;

		auto &atom = atoms[i];

		// If its a binary operation, we need to ensure
		// that the overload is legalized, at this stage
		// all the operations must have been validated
		// according to the GLSL specification
		if (auto operation = atom.get <Operation> ()) {
			// The operands are guaranteed to be
			// primitive types at this point

			std::vector <Index> args;
			args.push_back(operation->a);
			if (operation->b != -1)
				args.push_back(operation->b);

			std::vector <PrimitiveType> types;
			for (auto i : args) {
				auto ptype = primitive_type_of(buffer, i);
				// fmt::println("  {} -> {}", i, tbl_primitive_types[ptype]);
				types.push_back(ptype);
			}

			if (vector_type(types[0]) || vector_type(types[1])) {
				transformed = true;
				legalize_for_cc_operation_vector_overload(mapped[i],
					operation->code,
					args[0], args[1],
					types[0], types[1]);
			}
		}

		// Constructors for vectors may be using
		// a vector to vector constructor where
		// the fields are not mapped one-to-one
		// with the arguments provided
		if (auto constructor = atom.get <Construct> ()) {
			auto ptype = primitive_type_of(buffer, constructor->type);
			if (!(constructor->mode == global) && vector_type(ptype)) {
				auto args = buffer.expand_list(constructor->args);
				std::vector <PrimitiveType> types;
				for (auto i : args) {
					auto ptype = primitive_type_of(buffer, i);
					types.push_back(ptype);
				}

				if (types.size() == 1) {
					if (legalize_for_cc_vector_constructor(mapped[i], ptype, args, types))
						transformed = true;
				}
			}
		}

		// Some intrinsics need to be legalized, and
		// others are not supported for CC targets
		if (auto intrinsic = atom.get <Intrinsic> ()) {
			auto args = buffer.expand_list(intrinsic->args);
			std::vector <PrimitiveType> types;
			for (auto i : args) {
				auto ptype = primitive_type_of(buffer, i);
				types.push_back(ptype);
			}

			if (legalize_for_cc_intrinsic(mapped[i], intrinsic->opn, args, types))
				transformed = true;
		}

		if (!transformed)
			mapped[i].track(0);
	}

	// TODO: debug flag
	// fmt::println("stitch mapped blocks:");
	// for (size_t i = 0; i < mapped.size(); i++) {
	// 	fmt::println("--------------->> ({})", i);
	// 	fmt::println("masks:");
	// 	for (auto &m : mapped[i].refs)
	// 		fmt::println("  > ({}, {:02b})", m.index, m.mask);
	// 	mapped[i].dump();
	// }

	buffer = Buffer();
	stitch_mapped_instructions(buffer, mapped);
}

} // namespace jvl::thunder
