#include "thunder/atom.hpp"
#include "thunder/ad.hpp"
#include "thunder/opt.hpp"
#include "ire/emitter.hpp"

namespace jvl::thunder {

MODULE(ad-forward);

// Conventional synthesis will have the first type field be the
// primal value, and the secnod be the dual/derivative value
int synthesize_differential_type(const std::vector <Atom> &atoms, const TypeInformation &tf)
{
	auto &em = ire::Emitter::active;

	// Primitive types
	if (tf.down == -1 && tf.next == -1) {
		switch (tf.item) {
		case f32:
		{
			auto primal = tf;
			auto dual = tf;

			index_t id = em.emit(dual);
			primal.next = id;
			return em.emit(primal);
		};

		default:
			fmt::println("primitive type does not have a differential type: {}",
				tbl_primitive_types[tf.item]);
			abort();
		}
	}

	fmt::println("synthesis of differential types for structs is unsupported");
	abort();
}

// In the post-transform stitching process, there
// are two kinds of indices which need to be reindexed:
//
//   1. Local indices, which are references to atoms in
//	the same scratch atoms, and simply need to be offset
//
//   2. Global indices, which were derived from the original atom,
//      and need to be reindexed to the end of that atom's transformed
//	scratch atoms
//
// The latter is easier to keep track of, so we store addresses to those
// indices in a per-atom list after the atoms have been emitted.

// Each original instruction is mapped to:
//   - a scratch atoms of atoms as its transformed code
//   - a list of references to indices pointing to original atoms
// using ref_index_t = std::reference_wrapper <index_t>;

struct ad_fwd_iteration_context_t {
	std::deque <index_t> queue;
	std::unordered_set <index_t> diffed;
	const std::vector <Atom> &atoms;
};

index_t ad_fwd_binary_operation_dual_value(mapped_instruction_t &mapped,
					   const std::array <index_t, 2> &arg0,
					   const std::array <index_t, 2> &arg1,
					   OperationCode code,
					   index_t type)
{
	auto &em = ire::Emitter::active;

	switch (code) {

	case addition:
	case subtraction:
	{
		index_t dual_args = em.emit_list_chain(arg0[1], arg1[1]);
		index_t dual = em.emit(Operation(dual_args, code));
		mapped.track(dual, 0b10);
		return dual;
	}

	case multiplication:
	{
		index_t df_g = em.emit_list_chain(arg0[1], arg1[0]);
		index_t f_dg = em.emit_list_chain(arg0[0], arg1[1]);
		auto products = em.emit_sequence(Operation(df_g, multiplication),
						 Operation(f_dg, multiplication));
		index_t sum_args = em.emit_list_chain(products[0], products[1]);
		// TODO: track values?
		return em.emit(Operation(sum_args, addition));
	}

	case division:
	{
		index_t df_g = em.emit_list_chain(arg0[1], arg1[0]);
		index_t f_dg = em.emit_list_chain(arg0[0], arg1[1]);
		auto products = em.emit_sequence(Operation(df_g, multiplication),
						 Operation(f_dg, multiplication));
		index_t sub_args = em.emit_list_chain(products[0], products[1]);
		index_t numerator = em.emit(Operation(sub_args, subtraction));

		auto square_args = em.emit_list_chain(arg1[0], arg1[0]);
		index_t demoninator = em.emit(Operation(square_args, multiplication));

		auto div_args = em.emit_list_chain(numerator, demoninator);
		return em.emit(Operation(div_args, division));
	}

	default:
	{
		fmt::println("unsupported operation <{}> encountered in fwd AD",
			tbl_operation_code[code]);
		abort();
	}

	}

	return -1;
}

index_t ad_fwd_intrinsic_dual_value(mapped_instruction_t &mapped,
				    const std::array <index_t, 2> &arg0,
				    IntrinsicOperation opn,
				    index_t type)
{
	auto &em = ire::Emitter::active;

	auto chain_rule = [&](index_t result) -> index_t {
		index_t args = em.emit_list_chain(result, arg0[1]);
		result = em.emit(Operation(args, multiplication));
		mapped.track(result, 0b10);
		return result;
	};

	switch (opn) {

	case sin:
	{
		index_t args;
		index_t result;

		args = em.emit_list_chain(arg0[0]);
		result = em.emit(Intrinsic(args, type, cos));
		mapped.track(result, 0b10);

		return chain_rule(result);
	}

	case cos:
	{
		index_t args;
		index_t result;

		args = em.emit_list_chain(arg0[0]);
		result = em.emit(Intrinsic(args, type, sin));
		mapped.track(result, 0b10);

		args = em.emit_list_chain(result);
		result = em.emit(Operation(args, unary_negation));
		mapped.track(result, 0b10);

		return chain_rule(result);
	}

	default:
	{
		fmt::println("unsupported intrinsic <{}> encountered in fwd AD",
			tbl_intrinsic_operation[opn]);
		abort();
	}

	}

	return -1;
}

void ad_fwd_transform_instruction(ad_fwd_iteration_context_t &context,
				  mapped_instruction_t &mapped,
				  index_t i)
{
	auto &em = ire::Emitter::active;
	auto &atom = context.atoms[i];
	auto &diffed = context.diffed;
	auto &queue = context.queue;

	// Clear the currently present scratch and add it for recording
	mapped.clear();
	em.push(mapped, false);

	// fmt::print("atom: ");
	// dump_ir_operation(atom);
	// fmt::print("\n");

	switch (atom.index()) {

	// Promote the type field to the
	// corresponding differential type
	case Atom::type_index <TypeInformation> ():
	{
		auto tf = atom.template as <TypeInformation> ();

		diffed.insert(i);

		// Create the differentiated type
		synthesize_differential_type(context.atoms, tf);

		// Everything should be a local index by now
	} break;

	// Nothing to do for the global itself,
	// but for the differentiated parameters,
	// new types need to be instantiated
	case Atom::type_index <Qualifier> ():
	{
		auto global = atom.template as <Qualifier> ();
		if (global.kind == parameter) {
			// NOTE: assuming this a differentiated parameter...
			diffed.insert(i);

			// Dependencies are pushed front
			queue.push_front(global.underlying);
		}

		em.emit(global);

		mapped.track(0);
	} break;

	case Atom::type_index <Returns> ():
	{
		auto returns = atom.template as <Returns> ();

		// Dependencies; must have come from
		// args, so no need to revisit it
		queue.push_front(returns.type);

		diffed.insert(i);

		em.emit(returns);

		mapped.track(0);
	} break;

	case Atom::type_index <List> ():
	{
		auto &list = atom.template as <List> ();

		diffed.insert(i);

		// Dependencies
		queue.push_front(list.item);
		if (list.next != -1)
			queue.push_front(list.next);

		em.emit(list);

		mapped.track(0);
	} break;

	case Atom::type_index <Operation> ():
	{
		auto &opn = atom.template as <Operation> ();

		// Assuming that the primal version was semantically
		// correct, the only thing that needs to be done here
		// is to separate the operation for both primal and dual
		// counterparts, and then add any extra operations

		diffed.insert(i);

		// Dependencies
		queue.push_front(opn.a);
		if (opn.b != -1)
			queue.push_front(opn.b);

		// Get arguments (assuming binary)
		JVL_ASSERT(opn.b != -1,
			"forward differentiation of operations "
			"requires binary operations, instead got: {}",
			atom);

		// Now fill in the actuall operation
		auto arg0 = em.emit_sequence(Load(opn.a, 0), Load(opn.a, 1));
		auto arg1 = em.emit_sequence(Load(opn.b, 0), Load(opn.b, 1));
		index_t opn_primal_args = em.emit_list_chain(arg0[0], arg1[0]);

		index_t primal = em.emit(Operation(opn_primal_args, opn.code));
		index_t dual = ad_fwd_binary_operation_dual_value(mapped, arg0, arg1, opn.code, -1);

		mapped.track(arg0[0], 0b01);
		mapped.track(arg1[0], 0b01);
		mapped.track(arg0[1], 0b01);
		mapped.track(arg1[1], 0b01);
		mapped.track(primal, 0b10);

		// Dual type storage
		JVL_ABORT("unfinished implementation of autodiff forward for binary operations");

		auto tf = context.atoms[0].as <TypeInformation> ();
		// auto tf = context.atoms[opn.type].as <TypeInformation> ();

		Construct dual_ctor;
		dual_ctor.type = synthesize_differential_type(context.atoms, tf);
		dual_ctor.args = em.emit_list_chain(primal, dual);

		em.emit(dual_ctor);
	} break;

	case Atom::type_index <Intrinsic> ():
	{
		auto &intr = atom.as <Intrinsic> ();

		diffed.insert(i);

		// Dependencies
		queue.push_front(intr.args);

		// Get arguments (assuming binary)
		std::vector <index_t> args;

		index_t li = intr.args;
		while (li != -1) {
			Atom arg = context.atoms[li];
			if (!arg.is <List> ()) {
				fmt::println("expected opn args to be a list chain");
				abort();
			}

			List list = arg.as <List> ();
			args.push_back(list.item);

			li = list.next;
		}

		// Assuming unary intrinsic operations
		auto arg0 = em.emit_sequence(Load(args[0], 0), Load(args[0], 1));
		index_t arg0_p = em.emit_list_chain(arg0[0]);
		index_t primal = em.emit(Intrinsic(arg0_p, intr.type, intr.opn));
		index_t dual = ad_fwd_intrinsic_dual_value(mapped, arg0, intr.opn, intr.type);

		// Dual type storage
		auto tf = context.atoms[intr.type].as <TypeInformation> ();

		Construct dual_ctor;
		dual_ctor.type = synthesize_differential_type(context.atoms, tf);
		dual_ctor.args = em.emit_list_chain(primal, dual);

		em.emit(dual_ctor);

		mapped.track(arg0[0], 0b01);
		mapped.track(arg0[1], 0b01);
		mapped.track(primal, 0b10);
	} break;

	// Simple pass through for others which are
	// unaffected such as list chains
	default:
	{
		diffed.insert(i);
		em.emit(atom);
		mapped.track(0);
	} break;

	}

	em.pop();
}

void ad_fwd_transform(Buffer &result, const Buffer &source)
{
	auto &em = ire::Emitter::active;
	auto &atoms = source.atoms;

	// Map each atom to a potentially new list of atoms
	std::vector <mapped_instruction_t> mapped(source.pointer);

	// Marking each differentiable parameter
	ad_fwd_iteration_context_t context { .atoms = atoms };

	for (size_t i = 0; i < mapped.size(); i++) {
		auto &atom = atoms[i];
		if (auto global = atom.template get <Qualifier> ()) {
			if (global->kind == parameter)
				context.queue.push_back(i);
		}

		// Default population of scratches is preservation
		em.push(mapped[i], false);
		em.emit(atom);
		em.pop();
	}

	auto usage_graph = usage(source);

	// Transform all differentiable instructions and their relatives
	while (context.queue.size()) {
		index_t i = context.queue.front();
		context.queue.pop_front();

		if (context.diffed.count(i))
			continue;

		ad_fwd_transform_instruction(context, mapped[i], i);

		// Add all uses
		for (auto &index : usage_graph[i])
			context.queue.push_back(index);
	}

	// Everything that is untouched needs to be populated into the global refs
	for (size_t i = 0; i < mapped.size(); i++) {
		if (context.diffed.contains(i))
			continue;

		// These are the atoms which were simply forwarded in transformation
		mapped[i].track(0);
	}

	// Stitch the transformed blocks
	stitch_mapped_instructions(result, mapped);
}

} // namespace jvl::thunder
