#include "ire/emitter.hpp"
#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/linkage.hpp"
#include "thunder/opt.hpp"
#include "thunder/properties.hpp"

namespace jvl::thunder::detail {

MODULE(legalize-cc);

index_t instruction_type(const std::vector <Atom> &pool, index_t i)
{
	auto &atom = pool[i];

	switch (atom.index()) {
	case Atom::type_index <TypeInformation> ():
	case Atom::type_index <Swizzle> ():
		// For now we let swizzle be a special case
		return i;
	case Atom::type_index <Qualifier> ():
		return atom.as <Qualifier> ().underlying;
	case Atom::type_index <Primitive> ():
		return atom.as <Primitive> ().type;
	case Atom::type_index <Operation> ():
		return atom.as <Operation> ().type;
	case Atom::type_index <Intrinsic> ():
		return atom.as <Intrinsic> ().type;
	case Atom::type_index <Load> ():
	{
		auto &load = atom.as <Load> ();
		index_t t = instruction_type(pool, load.src);

		index_t idx = load.idx;
		while (idx > 0) {
			auto &atom = pool[t];
			log::assertion(atom.is <TypeInformation> (), __module__);

			TypeInformation type_field = atom.as <TypeInformation> ();
			t = type_field.next;
			idx--;
		}

		return t;
	}
	default:
		break;
	}

	log::abort(__module__, fmt::format("unhandle case in instruction_type: {}", atom));
	return bad;
}

// TODO: cache this while emitting
PrimitiveType primitive_type_of(const std::vector <Atom> &pool, index_t i)
{
	index_t t = instruction_type(pool, i);

	auto &atom = pool[t];
	log::assertion(atom.is <TypeInformation> (), __module__,
		fmt::format("result of instruction_type(...) "
			"is not a typefield: {}", atom));

	TypeInformation tf = atom.as <TypeInformation> ();

	return (tf.down == -1 && tf.next == -1) ? tf.item : bad;
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
	mapped.transformed.clear();
	em.push(mapped.transformed, false);

	fmt::println("legalizing operation {} for overload ({}, {})",
		tbl_operation_code[code],
		tbl_primitive_types[type_a],
		tbl_primitive_types[type_b]);

	// Assuming this is a binary instruction
	if (vector_type(type_a) && vector_type(type_b)) {
		PrimitiveType ctype = swizzle_type_of(type_a, SwizzleCode::x);
		size_t ccount = vector_component_count(type_a);

		assert(ctype == swizzle_type_of(type_b, SwizzleCode::x));
		assert(ccount == vector_component_count(type_b));

		std::vector <index_t> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t ca = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(ca, 0b01);

			index_t cb = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(cb, 0b01);

			index_t l = em.emit(List(cb, -1));
			l = em.emit(List(ca, l));

			index_t t = em.emit(TypeInformation(-1, -1, ctype));
			components[i] = em.emit(Operation(l, t, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_a));
		em.emit(Construct(t, l));

		// fmt::println("legalized code:");
		// mapped.transformed.dump();
		// assert(false);
	} else if (vector_type(type_a) && !vector_type(type_b)) {
		assert(type_b == swizzle_type_of(type_a, SwizzleCode::x));

		size_t ccount = vector_component_count(type_a);

		std::vector <index_t> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t c = em.emit(Swizzle(a, (SwizzleCode) i));
			mapped.track(c, 0b01);

			index_t l = em.emit(List(b, -1));
			mapped.track(l, 0b01);
			l = em.emit(List(c, l));

			index_t t = em.emit(TypeInformation(-1, -1, type_b));
			components[i] = em.emit(Operation(l, t, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_a));
		em.emit(Construct(t, l));
	} else if (!vector_type(type_a) && vector_type(type_b)) {
		assert(type_a == swizzle_type_of(type_b, SwizzleCode::x));

		size_t ccount = vector_component_count(type_b);

		std::vector <index_t> components(ccount);
		for (size_t i = 0; i < ccount; i++) {
			index_t c = em.emit(Swizzle(b, (SwizzleCode) i));
			mapped.track(c, 0b01);

			index_t l = em.emit(List(c, -1));
			l = em.emit(List(a, l));
			mapped.track(l, 0b01);

			index_t t = em.emit(TypeInformation(-1, -1, type_a));
			components[i] = em.emit(Operation(l, t, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeInformation(-1, -1, type_b));
		em.emit(Construct(t, l));
	} else {
		// TODO: legalize by converting to higher type
		assert(false);
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
	mapped.transformed.clear();
	em.push(mapped.transformed, false);

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
		JVL_INFO("legalizing dot product");

		PrimitiveType type_a = types[0];
		PrimitiveType type_b = types[1];

		assert(vector_type(type_a) && vector_type(type_b));
		assert(swizzle_type_of(type_a, SwizzleCode::x) == swizzle_type_of(type_b, SwizzleCode::x));
		assert(vector_component_count(type_a) == vector_component_count(type_b));

		// Reset and activate the scratch
		mapped.transformed.clear();
		em.push(mapped.transformed, false);

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
			products[i] = em.emit(Operation(l, t, multiplication));
		}

		index_t top = products[0];
		for (size_t i = 0; i < ccount - 1; i++) {
			index_t l = em.emit_list_chain(top, products[i + 1]);
			index_t t = em.emit(TypeInformation(-1, -1, ctype));
			top = em.emit(Operation(l, t, addition));
		}

		em.pop();
	}
}

void legalize_for_cc(Buffer &buffer)
{
	JVL_STAGE();

	auto &em = ire::Emitter::active;
	auto &pool = buffer.pool;

	std::vector <mapped_instruction_t> mapped(buffer.pointer);

	for (index_t i = 0; i < mapped.size(); i++) {
		auto &atom = pool[i];

		// Default population of scratches is preservation
		em.push(mapped[i].transformed, false);
		em.emit(atom);
		em.pop();
	}

	for (index_t i = 0; i < mapped.size(); i++) {
		bool transformed = false;

		auto &atom = pool[i];

		// If its a binary operation, we need to ensure
		// that the overload is legalized, at this stage
		// all the operations must have been validated
		// according to the GLSL specification
		if (auto operation = atom.get <Operation> ()) {
			// The operands are guaranteed to be
			// primitive types at this point
			// fmt::println("binary operation: {}", atom);

			auto args = buffer.expand_list(operation->args);
			// fmt::println("type for each args:");

			std::vector <PrimitiveType> types;
			for (auto i : args) {
				auto ptype = primitive_type_of(pool, i);
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
			auto ptype = primitive_type_of(pool, constructor->type);
			if (vector_type(ptype)) {
				fmt::println("legalizing constructor: {}", atom);
				fmt::println("constructor for vector type: {}", tbl_primitive_types[ptype]);
				size_t components = vector_component_count(ptype);
				auto args = buffer.expand_list(constructor->args);
				std::vector <PrimitiveType> types;
				for (auto i : args) {
					auto ptype = primitive_type_of(pool, i);
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
				auto ptype = primitive_type_of(pool, i);
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
