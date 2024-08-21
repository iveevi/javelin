#include <concepts>
#include <list>
#include <deque>

#include <fmt/format.h>

#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "profiles/targets.hpp"
#include "thunder/atom.hpp"

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

}

// Conventional synthesis will have the first type field be the primal value,
// and the secnod be the dual/derivative value
void synthesize_differential_type(const std::vector <thunder::Atom> &pool, const thunder::TypeField &tf)
{
	auto &em = Emitter::active;

	// Primitive types
	if (tf.down == -1 && tf.next == -1) {
		switch (tf.item) {
		case jvl::thunder::f32:
		{
			auto primal = tf;
			auto dual = tf;
			
			thunder::index_t id = em.emit(dual);
			primal.next = id;
			em.emit(primal);
		} return;
		default:
			fmt::println("primitive type does not have a differential type: {}",
				thunder::type_table[tf.item]);
			abort();
		}
	}

	fmt::println("synthesis of differential types for structs is unsupported");
	abort();
}

template <typename R, typename ... Args>
requires differentiable_type <R> && (differentiable_type <Args> && ...)
auto dfwd(const callable_t <R, Args...> &callable)
{
	using namespace thunder;

	callable_t <dual_t <R>, dual_t <Args>...> result;

	// TODO: the internals of this can be moved to a non-template function
	auto &em = Emitter::active;
	auto &pool = callable.pool;

	// Map each atom to a potentially new list of atoms
	std::vector <Scratch> promoted;
	promoted.resize(callable.pointer);

	// Marking each 
	std::deque <index_t> propogation;
	for (index_t i = 0; i < pool.size(); i++) {
		auto &atom = pool[i];
		if (auto global = atom.template get <Global> ()) {
			if (global->qualifier == Global::parameter)
				propogation.push_back(i);
		}

		// Default population of scratches is preservation
		em.push(promoted[i]);
		em.emit(atom);
		em.pop();
	}

	auto usage_graph = usage(pool);

	fmt::println("usage graph:");
	for (index_t i = 0; i < usage_graph.size(); i++) {
		fmt::print("    {:4d}: ", i);
		for (auto &j : usage_graph[i])
			fmt::print("{} ", j);
		fmt::print("\n");
	}

	std::unordered_set <index_t> diffed;
	while (propogation.size()) {
		index_t i = propogation.front();
		propogation.pop_front();

		if (diffed.count(i))
			continue;

		// Clear the currently present scratch as it will likely be overwritten
		promoted[i].clear();

		auto &atom = pool[i];

		fmt::println("prop index: {} (size={})", i, propogation.size());
		fmt::print("\t");
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
			em.push(promoted[i]);
			synthesize_differential_type(pool, tf);
			em.pop();
		} break;

		// Nothing to do for the global itself,
		// but for the differentiated parameters,
		// new types need to be instantiated
		case Atom::type_index <Global> ():
		{
			auto global = atom.template as <Global> ();
			if (global.qualifier == Global::parameter) {
				// NOTE: assuming this a differentiated parameter...
				diffed.insert(i);

				// Dependencies are pushed front
				propogation.push_front(global.type);
			}

			em.push(promoted[i]);
			em.emit(global);
			em.pop();
		} break;

		case Atom::type_index <Returns> ():
		{
			auto returns = atom.template as <Returns> ();

			// Dependencies
			propogation.push_front(returns.args);
			propogation.push_front(returns.type);

			diffed.insert(i);

			em.push(promoted[i]);
			em.emit(returns);
			em.pop();
		} break;

		// NOTE: the bulk goes into hanlding operations and intrinsics/callables

		// Simple pass through for others which are
		// unaffected such as list chains
		default:
		{
			em.push(promoted[i]);
			em.emit(atom);
			em.pop();
		} break;
		}

		// Add all uses
		for (auto &index : usage_graph[i])
			propogation.push_back(index);
	}

	// fmt::println("Promoted scratches");
	// for (index_t i = 0; i < promoted.size(); i++) {
	// 	fmt::print("[AD] for: ");
	// 	dump_ir_operation(pool[i]);
	// 	fmt::print("\n");

	// 	promoted[i].dump();
		
	// 	fmt::print("\n");
	// }

	// Stitch the independent scratches
	// TODO: still need to undergo reindexing
	for (auto &s : promoted) {
		for (size_t i = 0; i < s.pointer; i++)
			result.emit(s.pool[i]);
	}

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
	
	// auto shader = [&]() {
	// 	layout_in <float, 0> input;
	// 	layout_out <float, 0> output;
	//
	// 	output = dsquare(dual(input, f32(1.0f))).dual;
	// };
	//
	// kernel_from_args(shader).dump();
}
