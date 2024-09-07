#include "ire/emitter.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "thunder/linkage.hpp"
#include "logging.hpp"

namespace jvl::thunder::detail {

constexpr const char __module__[] = "legalize for cc";

index_t instruction_type(const std::vector <Atom> &pool, index_t i)
{
	auto &atom = pool[i];

	switch (atom.index()) {
	case Atom::type_index <TypeField> ():
	case Atom::type_index <Swizzle> ():
		// For now we let swizzle be a special case
		return i;
	case Atom::type_index <Global> ():
		return atom.as <Global> ().type;
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
			log::assertion(atom.is <TypeField> (), __module__);

			TypeField type_field = atom.as <TypeField> ();
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
	log::assertion(atom.is <TypeField> (), __module__,
		fmt::format("result of instruction_type(...) "
			"is not a typefield: {}", atom));

	TypeField tf = atom.as <TypeField> ();
	log::assertion(tf.item != bad, __module__,
		fmt::format("type field obtained from instruction_type(...) "
			"is not a primitive: {}", atom));

	return tf.item;
}

void legalize_for_jit_operation_vector_overload(mapped_instruction_t &mapped,
						OperationCode code,
						index_t a,
						index_t b,
						PrimitiveType type_a,
						PrimitiveType type_b)
{
	auto &em = ire::Emitter::active;

	// Reset and activate the scratch
	mapped.transformed.clear();
	em.push(mapped.transformed);

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

			index_t t = em.emit(TypeField(-1, -1, ctype));
			components[i] = em.emit(Operation(l, t, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeField(-1, -1, type_a));
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

			index_t t = em.emit(TypeField(-1, -1, type_b));
			components[i] = em.emit(Operation(l, t, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeField(-1, -1, type_a));
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

			index_t t = em.emit(TypeField(-1, -1, type_a));
			components[i] = em.emit(Operation(l, t, code));
		}

		index_t l = em.emit_list_chain(components);
		index_t t = em.emit(TypeField(-1, -1, type_b));
		em.emit(Construct(t, l));
	} else {
		// TODO: legalize by converting to higher type
		assert(false);
	}
}

void legalize_for_cc(Scratch &scratch)
{
	auto &em = ire::Emitter::active;
	auto &pool = scratch.pool;

	std::vector <mapped_instruction_t> mapped(scratch.pointer);

	for (index_t i = 0; i < mapped.size(); i++) {
		auto &atom = pool[i];

		// Default population of scratches is preservation
		em.push(mapped[i].transformed);
		em.emit(atom);
		em.pop();
	}

	auto list_args = [&](index_t i) {
		// TODO: util function
		std::vector <index_t> args;
		while (i != -1) {
			auto &atom = pool[i];
			assert(atom.is <List> ());

			List list = atom.as <List> ();
			args.push_back(list.item);

			i = list.next;
		}

		return args;
	};

	for (index_t i = 0; i < mapped.size(); i++) {
		bool transformed = false;

		auto &atom = pool[i];

		// If its a binary operation, we need to ensure
		// that the overload is legalized, at this stage
		// all the operations must have been validated
		// according to the GLSL specification
		if (auto op = atom.get <Operation> ()) {
			// The operands are guaranteed to be
			// primitive types at this point
			// fmt::println("binary operation: {}", atom);

			auto args = list_args(op->args);
			// fmt::println("type for each args:");

			std::vector <PrimitiveType> types;
			for (auto i : args) {
				auto ptype = primitive_type_of(pool, i);
				// fmt::println("  {} -> {}", i, tbl_primitive_types[ptype]);
				types.push_back(ptype);
			}

			if (vector_type(types[0]) || vector_type(types[1])) {
				transformed = true;
				legalize_for_jit_operation_vector_overload(mapped[i], op->code,
					args[0], args[1], types[0], types[1]);
			}
		}

		if (!transformed)
			mapped[i].track(0);
	}

	scratch = Scratch();
	stitch_mapped_instructions(scratch, mapped);
}

} // namespace jvl::thunder::detail
