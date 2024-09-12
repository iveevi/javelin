#include "ire/emitter.hpp"
#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "thunder/opt.hpp"
#include "thunder/properties.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder::detail {

MODULE(legalize-cc);

PrimitiveType primitive_type_of(const Buffer &buffer, index_t i)
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
					       index_t a,
					       index_t b,
					       PrimitiveType type_a,
					       PrimitiveType type_b)
{
	auto &em = ire::Emitter::active;

	// Reset and activate the scratch
	mapped.clear();
	em.push(mapped, false);

	fmt::println("legalizing operation {} for overload ({}, {})",
		tbl_operation_code[code],
		tbl_primitive_types[type_a],
		tbl_primitive_types[type_b]);

	// Assuming this is a binary instruction
	if (vector_type(type_a) && vector_type(type_b)) {
		PrimitiveType ctype = swizzle_type_of(type_a, SwizzleCode::x);
		size_t ccount = vector_component_count(type_a);

		JVL_ASSERT_PLAIN(ctype == swizzle_type_of(type_b, SwizzleCode::x));
		JVL_ASSERT_PLAIN(ccount == vector_component_count(type_b));

		std::vector <index_t> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t ca = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(ca, 0b01);

			index_t cb = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(cb, 0b01);

			index_t l = em.emit(List(cb, -1));
			l = em.emit(List(ca, l));

			index_t t = em.emit(TypeInformation(-1, -1, ctype));
			components[i] = em.emit(Operation(l, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_a));
		em.emit(Construct(t, l));

		// fmt::println("legalized code:");
		// mapped.transformed.dump();
		// JVL_ASSERT_PLAIN(false);
	} else if (vector_type(type_a) && !vector_type(type_b)) {
		JVL_ASSERT_PLAIN(type_b == swizzle_type_of(type_a, SwizzleCode::x));

		size_t ccount = vector_component_count(type_a);

		std::vector <index_t> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t c = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(c, 0b01);

			index_t l = em.emit(List(b, -1));
			mapped.track(l, 0b01);
			l = em.emit(List(c, l));

			index_t t = em.emit(TypeInformation(-1, -1, type_b));
			components[i] = em.emit(Operation(l, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_a));
		em.emit(Construct(t, l));
	} else if (!vector_type(type_a) && vector_type(type_b)) {
		JVL_ASSERT_PLAIN(type_a == swizzle_type_of(type_b, SwizzleCode::x));

		size_t ccount = vector_component_count(type_b);

		std::vector <index_t> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t c = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(c, 0b01);

			index_t l = em.emit(List(c, -1));
			l = em.emit(List(a, l));
			mapped.track(l, 0b01);

			index_t t = em.emit(TypeInformation(-1, -1, type_a));
			components[i] = em.emit(Operation(l, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_b));
		em.emit(Construct(t, l));
	} else {
		// TODO: legalize by converting to higher type
		JVL_ASSERT_PLAIN(false);
	}

	em.pop();
}

void legalize_for_cc_vector_constructor(mapped_instruction_t &mapped,
					PrimitiveType type_to_construct,
					const std::vector <index_t> &args,
					const std::vector <PrimitiveType> &types)
{
	fmt::println("legalizing constructor for {}", tbl_primitive_types[type_to_construct]);
	for (auto type : types)
		fmt::println("  arg: {}", tbl_primitive_types[type]);

	// For now assume only one argument when constructing heterogenously
	JVL_ASSERT(args.size() == 1, "vector constructor legalization currently requires exactly one argument");
	
	auto &em = ire::Emitter::active;
	
	// Reset and activate the scratch
	mapped.clear();
	em.push(mapped, false);

	PrimitiveType type_a = types[0];
	if (type_a == type_to_construct) {
		fmt::println("identical constructor");
		size_t ccount = vector_component_count(type_to_construct);

		std::vector <index_t> components;
		for (size_t i = 0; i < ccount; i++) {
			index_t c = em.emit(Swizzle(args[0], (SwizzleCode) i));
			mapped.track(c, 0b01);
			components.push_back(c);
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_to_construct));
		em.emit(Construct(t, l));
	} else {
		JVL_ABORT("unsupported argument {} when legalizing constructor for {}",
			tbl_primitive_types[type_a],
			tbl_primitive_types[type_to_construct]);
	}

	em.pop();
}

void legalize_for_cc_intrinsic(mapped_instruction_t &mapped,
			       IntrinsicOperation opn,
			       const std::vector <index_t> &args,
			       const std::vector <PrimitiveType> &types)
{
	// Intrinsics which can be successfully legalized
	static const std::unordered_set <IntrinsicOperation> legalizable {
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

		JVL_INFO("legalizing dot product between {} and {}",
			tbl_primitive_types[type_a],
			tbl_primitive_types[type_b]);

		JVL_ASSERT_PLAIN(vector_type(type_a) && vector_type(type_b));
		JVL_ASSERT_PLAIN(swizzle_type_of(type_a, SwizzleCode::x) == swizzle_type_of(type_b, SwizzleCode::x));
		JVL_ASSERT_PLAIN(vector_component_count(type_a) == vector_component_count(type_b));

		// Reset and activate the scratch
		mapped.clear();
		em.push(mapped, false);

		size_t ccount = vector_component_count(type_a);
		PrimitiveType ctype = swizzle_type_of(type_a, SwizzleCode::x);

		index_t a = args[0];
		index_t b = args[1];
		
		std::vector <index_t> products(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t ca = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(ca, 0b01);

			index_t cb = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(cb, 0b01);

			index_t l = em.emit(List(cb, -1));
			l = em.emit(List(ca, l));

			index_t t = em.emit(TypeInformation(-1, -1, ctype));
			products[i] = em.emit(Operation(l, multiplication));
		}

		index_t top = products[0];
		for (size_t i = 0; i < ccount - 1; i++) {
			index_t l = em.emit_list_chain(top, products[i + 1]);
			index_t t = em.emit(TypeInformation(-1, -1, ctype));
			top = em.emit(Operation(l, addition));
		}

		em.pop();
	}
}

// TODO: legalization context...
void legalize_for_cc(Buffer &buffer)
{
	JVL_STAGE();

	buffer.dump();

	auto &em = ire::Emitter::active;
	auto &atoms = buffer.atoms;

	std::vector <mapped_instruction_t> mapped(buffer.pointer);

	for (index_t i = 0; i < mapped.size(); i++) {
		auto &atom = atoms[i];

		// Default population of scratches is preservation
		em.push(mapped[i], false);
		em.emit(atom);
		em.pop();
	}

	for (index_t i = 0; i < mapped.size(); i++) {
		bool transformed = false;

		auto &atom = atoms[i];

		// If its a binary operation, we need to ensure
		// that the overload is legalized, at this stage
		// all the operations must have been validated
		// according to the GLSL specification
		if (auto operation = atom.get <Operation> ()) {
			// The operands are guaranteed to be
			// primitive types at this point

			std::vector <index_t> args;
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
			if (!constructor->transient && vector_type(ptype)) {
				fmt::println("legalizing constructor: {}", atom);
				fmt::println("constructor for vector type: {}", tbl_primitive_types[ptype]);
				size_t components = vector_component_count(ptype);
				auto args = buffer.expand_list(constructor->args);
				std::vector <PrimitiveType> types;
				for (auto i : args) {
					auto ptype = primitive_type_of(buffer, i);
					types.push_back(ptype);
				}

				legalize_for_cc_vector_constructor(mapped[i], ptype, args, types);
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

			legalize_for_cc_intrinsic(mapped[i], intrinsic->opn, args, types);
		}

		if (!transformed)
			mapped[i].track(0);
	}

	buffer = Buffer();
	stitch_mapped_instructions(buffer, mapped);
}

} // namespace jvl::thunder::detail
