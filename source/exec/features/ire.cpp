#include <concepts>
#include <deque>

#include <fmt/format.h>
#include <functional>

#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/util.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: pass structs as inout to callables
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again

using namespace jvl;
using namespace jvl::ire;

// Generative system for automatic differentiation
struct undefined_derivative_t {};

struct f32_derivative_t {
	f32 primal;
	f32 dual;

	auto layout() const {
		// TODO: this should be named
		return uniform_layout(primal, dual);
	}
};

template <size_t N>
struct vec_derivative_t {
	vec <float, N> primal;
	vec <float, N> dual;

	auto layout() const {
		return uniform_layout(primal, dual);
	}
};

template <typename T>
struct manifested_dual_type {
	using type = undefined_derivative_t;
};

template <>
struct manifested_dual_type <f32> {
	using type = f32_derivative_t;
};

template <size_t N>
struct manifested_dual_type <vec <float, N>> {
	using type = vec_derivative_t <N>;
};

template <generic T>
using dual_t = manifested_dual_type <T> ::type;

template <typename T>
concept differentiable_type = generic <dual_t <T>>;

template <generic T, typename U>
dual_t <T> dual(const T &p, const U &d)
{
	return dual_t <T> (p, d);
}

static_assert(differentiable_type <f32>);
static_assert(differentiable_type <vec2>);
static_assert(differentiable_type <vec3>);
static_assert(differentiable_type <vec4>);

namespace jvl::thunder {

using usage_list = std::vector <index_t>;

[[gnu::always_inline]]
inline bool uses(Atom atom, index_t i)
{
	auto &&addresses = atom.addresses();
	return (i != -1) && ((addresses.a0 == i) || (addresses.a1 == i));
}

usage_list usage(const std::vector <Atom> &pool, index_t index)
{
	usage_list indices;
	for (index_t i = index + 1; i < pool.size(); i++) {
		if (uses(pool[i], index))
			indices.push_back(i);
	}

	return indices;
}

using usage_graph = std::vector <usage_list>;

usage_graph usage(const Scratch &scratch)
{
	usage_graph graph(scratch.pointer);
	for (index_t i = 0; i < graph.size(); i++)
		graph[i] = usage(scratch.pool, i);

	return graph;
}

// Conventional synthesis will have the first type field be the
// primal value, and the secnod be the dual/derivative value
int synthesize_differential_type(const std::vector <Atom> &pool, const TypeField &tf)
{
	auto &em = Emitter::active;

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
//	the same scratch pool, and simply need to be offset
//
//   2. Global indices, which were derived from the original atom,
//      and need to be reindexed to the end of that atom's transformed
//	scratch pool
//
// The latter is easier to keep track of, so we store addresses to those
// indices in a per-atom list after the atoms have been emitted.

// Each original instruction is mapped to:
//   - a scratch pool of atoms as its transformed code
//   - a list of references to indices pointing to original atoms
// using ref_index_t = std::reference_wrapper <index_t>;

struct ref_index_t {
	index_t index;
	int8_t mask = 0b11;
};

struct ad_fwd_mapped_t {
	Scratch transformed;
	std::vector <ref_index_t> refs;

	Atom &operator[](size_t index) {
		return transformed.pool[index];
	}

	void track(index_t index, int8_t mask = 0b11) {
		refs.push_back(ref_index_t(index, mask));
	}
};

struct ad_fwd_iteration_context_t {
	std::deque <index_t> queue;
	std::unordered_set <index_t> diffed;
	const std::vector <Atom> &pool;
};

index_t ad_fwd_binary_operation_dual_value
(
	ad_fwd_mapped_t &mapped,
	const std::array <index_t, 2> &arg0,
	const std::array <index_t, 2> &arg1,
	OperationCode code,
	index_t type
)
{
	auto &em = Emitter::active;

	switch (code) {
	
	case addition:
	case subtraction:
	{
		index_t dual_args = em.emit_list_chain(arg0[1], arg1[1]);
		index_t dual = em.emit(Operation(dual_args, type, code));
		mapped.track(dual, 0b10);
		return dual;
	}

	case multiplication:
	{
		index_t base_type = em.emit(TypeField(-1, -1, f32));
		index_t df_g = em.emit_list_chain(arg0[1], arg1[0]);
		index_t f_dg = em.emit_list_chain(arg0[0], arg1[1]);
		auto products = em.emit_sequence(Operation(df_g, base_type, multiplication),
						 Operation(f_dg, base_type, multiplication));
		index_t sum_args = em.emit_list_chain(products[0], products[1]);
		// TODO: track values?
		return em.emit(Operation(sum_args, base_type, addition));
	}

	case division:
	{
		index_t base_type = em.emit(TypeField(-1, -1, f32));
		index_t df_g = em.emit_list_chain(arg0[1], arg1[0]);
		index_t f_dg = em.emit_list_chain(arg0[0], arg1[1]);
		auto products = em.emit_sequence(Operation(df_g, base_type, multiplication),
						 Operation(f_dg, base_type, multiplication));
		index_t sub_args = em.emit_list_chain(products[0], products[1]);
		index_t numerator = em.emit(Operation(sub_args, base_type, subtraction));
		
		auto square_args = em.emit_list_chain(arg1[0], arg1[0]);
		index_t demoninator = em.emit(Operation(square_args, base_type, multiplication));

		auto div_args = em.emit_list_chain(numerator, demoninator);
		return em.emit(Operation(div_args, base_type, division));
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

index_t ad_fwd_intrinsic_dual_value
(
	ad_fwd_mapped_t &mapped,
	const std::array <index_t, 2> &arg0,
	IntrinsicOperation opn,
	index_t type
)
{
	auto &em = Emitter::active;

	switch (opn) {
	
	case sin:
	{
		index_t args = em.emit_list_chain(arg0[1]);
		index_t result = em.emit(Intrinsic(args, type, cos));
		mapped.track(result, 0b10);
		return result;
	}
	
	case cos:
	{
		index_t args;
		index_t result;

		args = em.emit_list_chain(arg0[1]);
		result = em.emit(Intrinsic(args, type, sin));
		mapped.track(result, 0b10);

		args = em.emit_list_chain(result);
		result = em.emit(Operation(args, type, unary_negation));
		mapped.track(result, 0b10);

		return result;
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

void ad_fwd_transform_instruction
(
	ad_fwd_iteration_context_t &context,
	ad_fwd_mapped_t &mapped,
	index_t i
)
{
	auto &em = Emitter::active;
	auto &atom = context.pool[i];
	auto &diffed = context.diffed;
	auto &queue = context.queue;
	auto &refs = mapped.refs;

	// Clear the currently present scratch and add it for recording
	mapped.transformed.clear();
	em.push(mapped.transformed);

	fmt::print("atom: ");
	dump_ir_operation(atom);
	fmt::print("\n");

	switch (atom.index()) {

	// Promote the type field to the
	// corresponding differential type
	case Atom::type_index <TypeField> ():
	{
		auto tf = atom.template as <TypeField> ();

		diffed.insert(i);

		// Create the differentiated type
		synthesize_differential_type(context.pool, tf);

		// Everything should be a local index by now
	} break;

	// Nothing to do for the global itself,
	// but for the differentiated parameters,
	// new types need to be instantiated
	case Atom::type_index <Global> ():
	{
		auto global = atom.template as <Global> ();
		if (global.qualifier == GlobalQualifier::parameter) {
			// NOTE: assuming this a differentiated parameter...
			diffed.insert(i);

			// Dependencies are pushed front
			queue.push_front(global.type);
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
		queue.push_front(opn.args);

		// Get arguments (assuming binary)
		std::vector <index_t> args;

		index_t li = opn.args;
		while (li != -1) {
			Atom arg = context.pool[li];
			if (!arg.is <List> ()) {
				fmt::println("expected opn args to be a list chain");
				abort();
			}

			List list = arg.as <List> ();
			args.push_back(list.item);

			li = list.next;
		}
		
		// Now fill in the actuall operation
		auto arg0 = em.emit_sequence(Load(args[0], 0), Load(args[0], 1));
		auto arg1 = em.emit_sequence(Load(args[1], 0), Load(args[1], 1));
		index_t opn_primal_args = em.emit_list_chain(arg0[0], arg1[0]);

		index_t primal = em.emit(Operation(opn_primal_args, opn.type, opn.code));
		index_t dual = ad_fwd_binary_operation_dual_value(mapped, arg0, arg1, opn.code, opn.type);

		mapped.track(arg0[0], 0b01);
		mapped.track(arg1[0], 0b01);
		mapped.track(arg0[1], 0b01);
		mapped.track(arg1[1], 0b01);
		mapped.track(primal, 0b10);

		// Dual type storage
		auto tf = context.pool[opn.type].as <TypeField> ();

		Construct dual_ctor;
		dual_ctor.type = synthesize_differential_type(context.pool, tf);
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
			Atom arg = context.pool[li];
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
		auto tf = context.pool[intr.type].as <TypeField> ();

		Construct dual_ctor;
		dual_ctor.type = synthesize_differential_type(context.pool, tf);
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

void ad_fwd_transform_stitch_mapped(Scratch &result, std::vector <ad_fwd_mapped_t> &mapped)
{
	// Reindex locals by offset
	std::vector <size_t> block_offsets;

	size_t offset = 0;
	for (index_t i = 0; i < mapped.size(); i++) {
		auto &s = mapped[i].transformed;
		auto &g = mapped[i].refs;

		// Create a map which offsets
		wrapped::reindex <index_t> reindex;
		for (size_t i = 0; i < s.pointer; i++)
			reindex[i] = i + offset;

		// Preserve global refs
		struct ref_state_t : ref_index_t {
			index_t v0 = -1;
			index_t v1 = -1;
		};

		auto ref_state_from = [&](const ref_index_t &ri) {
			ref_state_t state(ri);
			auto &&addrs = s.pool[ri.index].addresses();
			state.v0 = addrs.a0;
			state.v1 = addrs.a1;
			return state;
		};

		auto ref_state_restore = [&](const ref_state_t &state) {
			auto &&addrs = s.pool[state.index].addresses();
			if (state.v0 != -1 && (state.mask & 0b01) == 0b01)
				addrs.a0 = state.v0;
			if (state.v1 != -1 && (state.mask & 0b10) == 0b10)
				addrs.a1 = state.v1;
		};

		std::vector <ref_state_t> store;
		for (auto &r : g)
			store.emplace_back(ref_state_from(r));

		// Reindex each atom
		for (size_t i = 0; i < s.pointer; i++)
			reindex_ir_operation(reindex, s.pool[i]);

		// Restore global refs
		for (index_t i = 0; i < store.size(); i++)
			ref_state_restore(store[i]);

		offset += s.pointer;

		block_offsets.push_back(offset - 1);
	}

	// Reindex the globals; doing it after because
	// some instructions (e.g. branches/loops) have
	// forward looking addresses
	for (index_t i = 0; i < mapped.size(); i++) {
		auto &s = mapped[i].transformed;
		auto &g = mapped[i].refs;

		for (auto &r : g) {
			auto &&addrs = s.pool[r.index].addresses();
			if (addrs.a0 != -1 && (r.mask & 0b01) == 0b01)
				addrs.a0 = block_offsets[addrs.a0];
			if (addrs.a1 != -1 && (r.mask & 0b10) == 0b10)
				addrs.a1 = block_offsets[addrs.a1];
		}
	}

	// Stitch the independent scratches
	for (auto &m : mapped) {
		auto &s = m.transformed;
		for (size_t i = 0; i < s.pointer; i++)
			result.emit(s.pool[i]);
	}
}

void ad_fwd_transform(Scratch &result, const Scratch &source)
{
	auto &em = Emitter::active;
	auto &pool = source.pool;

	// Map each atom to a potentially new list of atoms
	std::vector <ad_fwd_mapped_t> mapped(source.pointer);

	// Marking each differentiable parameter
	ad_fwd_iteration_context_t context { .pool = pool };

	for (index_t i = 0; i < mapped.size(); i++) {
		auto &atom = pool[i];
		if (auto global = atom.template get <Global> ()) {
			if (global->qualifier == GlobalQualifier::parameter)
				context.queue.push_back(i);
		}

		// Default population of scratches is preservation
		em.push(mapped[i].transformed);
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
	for (index_t i = 0; i < mapped.size(); i++) {
		if (context.diffed.contains(i))
			continue;

		// These are the atoms which were simply forwarded in transformation
		mapped[i].track(0);
	}

	// Stitch the transformed blocks
	ad_fwd_transform_stitch_mapped(result, mapped);
}

}

template <typename R, typename ... Args>
requires differentiable_type <R> && (differentiable_type <Args> && ...)
auto dfwd(const callable_t <R, Args...> &callable)
{
	callable_t <dual_t <R>, dual_t <Args>...> result;
	thunder::ad_fwd_transform(result, callable);
	return result.named("__dfwd_" + callable.name);
}

// Sandbox application
f32 __id(f32 x, f32 y)
{
	return cos(x);
	// return (x / y) + x;
}

auto id = callable(__id).named("id");

int main()
{
	id.dump();
	auto did = dfwd(id);
	did.dump();

	auto shader = [&]() {
		layout_in <float> input(0);
		layout_out <float> output(0);

		dual_t <f32> dx = dual(id(input, 1), f32(1.0f));
		dual_t <f32> dy = dual(id(1, (f32) input / 2.0f), input);

		output = did(dx, dy).dual;
	};

	auto kernel = kernel_from_args(shader);

	// kernel.dump();

	fmt::println("{}", kernel.synthesize(profiles::opengl_450));
}
