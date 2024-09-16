#include "ire/emitter.hpp"
#include "thunder/ad.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {

MODULE(ad-forward);

struct dual_atom {
	index_t primal;
	index_t dual;

	static dual_atom load(index_t source) {
		auto &em = ire::Emitter::active;

		dual_atom da;
		da.primal = em.emit_load(source, 0);
		da.dual = em.emit_load(source, 1);
		return da;
	}
};

// Conventional synthesis will have the first type field be the
// primal value, and the second be the dual/derivative value
index_t synthesize_dual_type(const QualifiedType &qt)
{
	auto &em = ire::Emitter::active;

	JVL_ASSERT(qt.is_primitive(), "only dual types of primitives are supported, got: {}", qt);

	auto &pd = qt.as <PlainDataType> ();

	PrimitiveType primitive = pd.as <PrimitiveType> ();
	index_t dual = em.emit_type_information(-1, -1, primitive);
	return em.emit_type_information(-1, dual, primitive);
}

struct ad_fwd_iteration_context_t : Buffer {
	std::deque <index_t> queue;
	std::unordered_set <index_t> diffed;

	ad_fwd_iteration_context_t(const Buffer &buffer) : Buffer(buffer) {}
};

index_t ad_fwd_binary_operation_dual_value(mapped_instruction_t &mapped,
					   const dual_atom &f,
					   const dual_atom &g,
					   OperationCode code)
{
	auto &em = ire::Emitter::active;

	switch (code) {

	case addition:
	case subtraction:
		return em.emit_operation(f.dual, g.dual, code);

	case multiplication:
	{
		// TODO: emit tags (comments)
		index_t df_g = em.emit_operation(f.dual, g.primal, multiplication);
		index_t f_dg = em.emit_operation(f.primal, g.dual, multiplication);
		return em.emit_operation(df_g, f_dg, addition);
	}

	case division:
	{
		index_t df_g = em.emit_operation(f.dual, g.primal, multiplication);
		index_t f_dg = em.emit_operation(f.primal, g.dual, multiplication);
		index_t n = em.emit_operation(df_g, f_dg, subtraction);
		index_t d = em.emit_operation(g.primal, g.primal, multiplication);
		return em.emit_operation(n, d, division);
	}

	default:
		break;
	}

	JVL_ABORT("unhandled operation $({}) in forward autodiff", tbl_operation_code[code]);
}

index_t ad_fwd_intrinsic_dual_value(mapped_instruction_t &mapped,
				    const std::array <index_t, 2> &arg0,
				    IntrinsicOperation opn,
				    index_t type)
{
	auto &em = ire::Emitter::active;

	auto chain_rule = [&](index_t result) -> index_t {
		result = em.emit_operation(result, arg0[1], multiplication);
		mapped.track(result, 0b10);
		return result;
	};

	switch (opn) {

	case sin:
	{
		index_t args;
		index_t result;

		args = em.emit_list_chain(arg0[0]);
		result = em.emit_intrinsic(args, type, cos);
		mapped.track(result, 0b10);

		return chain_rule(result);
	}

	case cos:
	{
		index_t args = em.emit_list_chain(arg0[0]);
		index_t result = em.emit_intrinsic(args, type, sin);
		mapped.track(result, 0b10);

		result = em.emit_operation(result, -1, unary_negation);
		mapped.track(result, 0b10);

		return chain_rule(result);
	}

	default:
		break;
	}
	
	JVL_ABORT("unhandled intrinsic $({}) in forward autodiff", tbl_intrinsic_operation[opn]);
}

void ad_fwd_transform_instruction(ad_fwd_iteration_context_t &context,
				  mapped_instruction_t &mapped,
				  index_t index)
{
	auto &em = ire::Emitter::active;
	auto &atom = context.atoms[index];
	auto &diffed = context.diffed;
	auto &queue = context.queue;

	// Clear the currently present scratch and add it for recording
	mapped.clear();
	em.push(mapped, false);

	switch (atom.index()) {

	// Promote the type field to the
	// corresponding differential type
	case Atom::type_index <TypeInformation> ():
	{
		// Create the differentiated type
		synthesize_dual_type(context.types[index]);

		// Everything should be a local index by now
		diffed.insert(index);
	} break;

	// Nothing to do for the global itself,
	// but for the differentiated parameters,
	// new types need to be instantiated
	case Atom::type_index <Qualifier> ():
	{
		auto global = atom.template as <Qualifier> ();
		if (global.kind == parameter) {
			// NOTE: assuming this a differentiated parameter...
			diffed.insert(index);

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

		diffed.insert(index);

		em.emit(returns);

		mapped.track(0);
	} break;

	case Atom::type_index <List> ():
	{
		auto &list = atom.template as <List> ();

		diffed.insert(index);

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

		diffed.insert(index);

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
		auto a = dual_atom::load(opn.a);
		auto b = dual_atom::load(opn.b);

		mapped.track(a.primal, 0b01);
		mapped.track(a.dual, 0b01);
		
		mapped.track(b.primal, 0b01);
		mapped.track(b.dual, 0b01);

		index_t primal = em.emit_operation(a.primal, b.primal, opn.code);
		index_t dual = ad_fwd_binary_operation_dual_value(mapped, a, b, opn.code);

		index_t type = synthesize_dual_type(context.types[index]);
		index_t args = em.emit_list_chain(primal, dual);
		em.emit_construct(type, args, false);
	} break;

	case Atom::type_index <Intrinsic> ():
	{
		JVL_ABORT("unfinished implementation of forward AD for intrinsics");
	} break;

	// Simple pass through for others which are
	// unaffected such as list chains
	default:
	{
		diffed.insert(index);
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
	ad_fwd_iteration_context_t context(source);

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
