#include <fmt/format.h>

#include <type_traits>
#include <vector>

#include "ire/core.hpp"
#include "ire/solidify.hpp"
#include "ire/type_synthesis.hpp"
#include "ire/uniform_layout.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"

// TODO: immutability for shader inputs types
// TODO: demote variables to inline if they are not modified later
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: std.hpp for additional features
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types

using namespace jvl;
using namespace jvl::ire;

template <size_t N, typename T, typename ... Args>
struct aggregate_base : aggregate_base <N + 1, Args...> {
protected:
	T v = T();
};

template <size_t N, typename T>
struct aggregate_base <N, T> {
protected:
	T v = T();
};

template <typename ... Args>
struct aggregate : aggregate_base <0, Args...> {

};

// Sandbox application
struct shuffle_pair {
	i32 x;
	i32 y;

	auto layout() const {
		return uniform_layout(
			field <"x"> (x),
			field <"y"> (y)
		);

		// TODO: named uniform layouts...
		// return uniform_layout(x, y);
	}

	using ire_value_type = struct {
		int x;
		int y;
	};
};

i32 __ftn(shuffle_pair sp)
{
	return sp.x;
	// sp.x = sp.x >> 5;
	// sp.y = sp.y << 17;
	// sp.x = (sp.x & 0xff) | (sp.y & 0b10101);
	// return sp.x & sp.y;
}

auto id = callable(__ftn).named("ftn");

template <typename T>
constexpr void jit_check_return()
{
	static_assert(solidifiable <T>, "function to be JIT-ed must have a return type that is solidifiable");
}

template <typename T, typename ... Args>
constexpr void jit_check_arguments()
{
	static_assert(solidifiable <T>, "function to be JIT-ed must have a arguments that are solidifiable");
	if constexpr (solidifiable <T> && sizeof...(Args))
		return jit_check_arguments <Args...> ();
}

template <typename R, typename ... Args>
auto jit(const callable_t <R, Args...> &callable)
{
	jit_check_return <R> ();
	jit_check_arguments <Args...> ();

	using function_t = solid_t <R> (*)(solid_t <Args> ...);
	auto kernel = callable.export_to_kernel();
	auto linkage = kernel.linkage().resolve();
	auto jr = linkage.generate_jit_gcc();
	return function_t(jr.result);
}

int main()
{
	// auto kernel = id.export_to_kernel();
	// auto linkage = kernel.linkage().resolve();
	// std::string source = linkage.generate_cplusplus();
	// fmt::println("{}", source);

	thunder::opt_transform_compact(id);
	// TODO: recursive dead code elimination in one pass...
	thunder::opt_transform_dead_code_elimination(id);
	thunder::opt_transform_dead_code_elimination(id);
	thunder::opt_transform_dead_code_elimination(id);
	thunder::opt_transform_dead_code_elimination(id);
	thunder::opt_transform_dead_code_elimination(id);
	thunder::opt_transform_dead_code_elimination(id);
	id.dump();

	auto v = solid_t <shuffle_pair> ();
	v.x = 123;
	v.y = 245;

	auto vv = std::make_tuple(123, 245);

	aggregate <int, int> vvv;

	auto jit_ftn = jit(id);
	auto jit_tuple = (int (*)(std::tuple <int, int>)) jit_ftn;
	auto jit_aggr = (int (*)(aggregate <int, int>)) jit_ftn;
	fmt::println("ftn(...) = {}", jit_ftn(v));
	fmt::println("ftn(...) = {}", jit_tuple(vv));
	fmt::println("ftn(...) = {}", jit_aggr(vvv));
}
