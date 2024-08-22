#include <concepts>
#include <deque>

#include <fmt/format.h>
#include <functional>

#include "ire/core.hpp"
#include "ire/emitter.hpp"
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

template <generic T>
dual_t <T> dual(const T &p, const T &d)
{
	return dual_t <T> (p, d);
}

template <non_trivial_generic T, generic U>
requires std::convertible_to <U, T>
dual_t <T> dual(const T &p, const U &d)
{
	return dual_t <T> (p, T(d));
}

template <generic T, non_trivial_generic U>
requires std::convertible_to <T, U>
dual_t <U> dual(const T &p, const U &d)
{
	return dual_t <U> (U(p), d);
}

static_assert(differentiable_type <f32>);
static_assert(differentiable_type <vec2>);
static_assert(differentiable_type <vec3>);
static_assert(differentiable_type <vec4>);

namespace jvl::thunder {

using usage_list = std::vector <index_t>;

bool uses(Atom atom, index_t i)
{
	switch (atom.index()) {
	case Atom::type_index <Load> ():
	{
		return (atom.as <Load> ().src == i);
	}

	case Atom::type_index <List> ():
	{
		auto list = atom.as <List> ();
		return (list.item == i) || (list.next == i);
	}

	case Atom::type_index <Global> ():
	{
		return (atom.as <Global> ().type == i);
	}

	case Atom::type_index <Returns> ():
	{
		auto returns = atom.as <Returns> ();
		return (returns.type == i) || (returns.args == i);
	}

	default:
		break;
	}

	return false;
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

usage_graph usage(const std::vector <Atom> &pool)
{
	usage_graph graph(pool.size());
	for (index_t i = 0; i < graph.size(); i++)
		graph[i] = usage(pool, i);

	return graph;
}

// Conventional synthesis will have the first type field be the
// primal value, and the secnod be the dual/derivative value
void synthesize_differential_type(const std::vector <Atom> &pool, const TypeField &tf)
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
			em.emit(primal);
		} return;

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
using ref_index_t = std::reference_wrapper <index_t>;

struct ad_fwd_mapped_t {
	Scratch transformed;
	std::vector <ref_index_t> refs;

	Atom &operator[](size_t index) {
		return transformed.pool[index];
	}

	void track(thunder::Addresses &&addrs) {
		if (addrs.a0 != -1) refs.push_back(std::ref(addrs.a0));
		if (addrs.a1 != -1) refs.push_back(std::ref(addrs.a1));
	}
};

struct ad_fwd_iteration_context_t {
	std::deque <index_t> queue;
	std::unordered_set <index_t> diffed;
	const std::vector <Atom> &pool;
};

void ad_fwd_transform_instruction(ad_fwd_iteration_context_t &context,
                		  ad_fwd_mapped_t &mapped,
				  index_t i)
{
	auto &em = Emitter::active;
	auto &atom = context.pool[i];
	auto &diffed = context.diffed;
	auto &queue = context.queue;
	auto &refs = mapped.refs;

	// Clear the currently present scratch and add it for recording
	mapped.transformed.clear();
	em.push(mapped.transformed);

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

		mapped.track(mapped[0].addresses());
	} break;

	case Atom::type_index <Returns> ():
	{
		auto returns = atom.template as <Returns> ();

		// Dependencies
		queue.push_front(returns.args);
		queue.push_front(returns.type);

		diffed.insert(i);

		em.emit(returns);

		mapped.track(mapped[0].addresses());
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

		mapped.track(mapped[0].addresses());
	} break;

	// NOTE: the bulk goes into hanlding operations and intrinsics/callables

	// Simple pass through for others which are
	// unaffected such as list chains
	default:
	{
		diffed.insert(i);
		em.emit(atom);
	} break;

	}
	
	return em.pop();
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
		// TODO: just use the addresses()
		wrapped::reindex <index_t> reindex;
		for (size_t i = 0; i < s.pointer; i++)
			reindex[i] = i + offset;

		// Preserve global refs
		std::vector <index_t> store;
		for (auto &r : g)
			store.push_back(r.get());

		// Reindex each atom
		for (size_t i = 0; i < s.pointer; i++)
			reindex_ir_operation(reindex, s.pool[i]);

		// Restore global refs
		for (index_t i = 0; i < store.size(); i++)
			g[i].get() = store[i];

		offset += s.pointer;

		block_offsets.push_back(offset - 1);
	}

	// Reindex the globals; doing it after because
	// some instructions (e.g. branches/loops) have
	// forward looking addresses
	for (index_t i = 0; i < mapped.size(); i++) {
		auto &g = mapped[i].refs;

		for (auto &r : g) {
			index_t p = r.get();
			r.get() = block_offsets[p];
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

	for (index_t i = 0; i < pool.size(); i++) {
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

	auto usage_graph = usage(pool);

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
		mapped[i].track(mapped[i][0].addresses());
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
f32 __id(f32 x)
{
	return x;
}

auto id = callable(__id).named("id");

int main()
{
	id.dump();
	auto did = dfwd(id);
	did.dump();

	auto shader = [&]() {
		// TODO: remove binding in the template, make it layout_in <float> input(0)
		layout_in <float, 0> input;
		layout_out <float, 0> output;

		output = did(dual(id(input), f32(1.0f))).dual;
	};

	auto kernel = kernel_from_args(shader);

	kernel.dump();

	fmt::println("{}", kernel.synthesize(profiles::opengl_450));
}
