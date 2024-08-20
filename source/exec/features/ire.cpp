#include <concepts>
#include <list>
#include <deque>

#include <fmt/format.h>

#include "ire/core.hpp"
#include "profiles/targets.hpp"

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

// Sandbox application
f32 __square(f32 x)
{
	return x;
}

auto square = callable(__square).named("square");

template <typename R, typename ... Args>
requires differentiable_type <R> && (differentiable_type <Args> && ...)
auto dfwd(const callable_t <R, Args...> &callable)
{
	callable_t <dual_t <R>, dual_t <Args>...> fwd;

	auto &pool = callable.pool;

	// Map each atom to a potentially new list of atoms
	std::vector <std::list <thunder::Atom>> promoted;
	promoted.resize(callable.pointer);

	std::deque <thunder::index_t> propogation;
	for (thunder::index_t i = 0; i < pool.size(); i++) {
		auto &atom = pool[i];
		if (auto global = atom.template get <thunder::Global> ()) {
			if (global->qualifier == thunder::Global::parameter)
				propogation.push_back(i);
		}
	}

	// TODO: build the usage graph...

	std::unordered_set <thunder::index_t> diffed;
	while (propogation.size()) {
		thunder::index_t i = propogation.front();
		propogation.pop_front();

		auto &atom = pool[i];
		if (auto global = atom.template get <thunder::Global> ()) {
			if (global->qualifier == thunder::Global::parameter) {
				diffed.insert(i);

				// Dependencies are pushed front
				propogation.push_front(global->type);
			}
		}

		fmt::println("prop index: {} (size={})", i, propogation.size());

		// Add all uses...
	}

	// Combine the generated lists

	return fwd.named("__dfwd_" + callable.name);
}

int main()
{
	square.dump();

	auto dsquare = dfwd(square);

	dsquare.dump();

	auto shader = [&]() {
		layout_in <float, 0> input;
		layout_out <float, 0> output;

		output = dsquare(dual(input, f32(1.0f))).dual;
	};

	kernel_from_args(shader).dump();
}
