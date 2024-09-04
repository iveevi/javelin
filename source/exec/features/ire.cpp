#include <fmt/format.h>

#include <type_traits>
#include <vector>

#include "ire/core.hpp"
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

// Sandbox application
f32 __ftn(f32 x, f32 y)
{
	return sin(cos(x) - y);
}

auto id = callable(__ftn).named("ftn");

template <typename R, typename ... Args>
auto jit(const callable_t <R, Args...> &callable)
{
	using function_t = solid_t <R> (*)(solid_t <Args> ...);
	auto kernel = callable.export_to_kernel();
	auto linkage = kernel.linkage().resolve();
	auto jr = linkage.generate_jit_gcc();
	return function_t(jr.result);
}

struct lighting {
	vec3 direction;
	vec3 color;

	auto layout() const {
		return uniform_layout(direction, color);
	}
};

struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;

	lighting light;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(model, view, proj, light);
	}
};

tuple_index <0> m_model;
tuple_index <1> m_view;
tuple_index <2> m_proj;
tuple_index <3> m_light;

tuple_index <0> m_direction;
tuple_index <1> m_color;

auto t = solid_t <mvp> ();
float x = t[m_light][m_direction].x;

static_assert(std::same_as <std::decay_t <decltype(t[m_model])>, float4x4>);
static_assert(std::same_as <std::decay_t <decltype(t[m_view])>, float4x4>);
static_assert(std::same_as <std::decay_t <decltype(t[m_proj])>, float4x4>);
// static_assert(std::same_as <std::decay_t <decltype(t[m_light][m_direction])>, float3>);
// static_assert(std::same_as <std::decay_t <decltype(t[m_light][m_color])>, float3>);

static_assert(sizeof(solid_t <mvp>) == 3 * sizeof(float4x4) + 2 * sizeof(float4));

int main()
{
	thunder::opt_transform_compact(id);
	thunder::opt_transform_dead_code_elimination(id);
	id.dump();

	auto jit_ftn = jit(id);
	fmt::println("result: {}", jit_ftn(1, 1));
}