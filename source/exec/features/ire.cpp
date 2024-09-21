// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <queue>
#include <map>
#include <set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

struct seed {
	u32 a;
	u32 b;

	auto layout() const {
		return uniform_layout("seed",
			named_field(a),
			named_field(b));
	}
};

auto seed_to_vector = callable_info() >> [](const seed &s)
{
	layout_in <u32, flat> bias(0);
	u32 x = (s.a << s.b) & (s.b - s.a) + bias;
	u32 y = (s.b + s.a * s.b)/s.a;
	uvec2 v = uvec2(x, y);
	return uintBitsToFloat(v);
};

struct aux {
	vec3 one;
	vec3 two;
	vec3 three;

	auto layout() const {
		return uniform_layout("aux",
			named_field(one),
			named_field(two),
			named_field(three));
	}
};

auto seed_to_angle = callable_info() >> [](const aux &a, const seed &s)
{
	sampler1D tex(1);
	layout_out <u32, flat> result(0);
	vec2 v = seed_to_vector(s);
	vec3 vone = v.x * a.one + tex.fetch(0, 0).x;
	vec3 vtwo = v.y * a.two;
	result = floatBitsToUint(vtwo).x;
	return dot(vone, vtwo);
};

int main()
{
	thunder::opt_transform(seed_to_vector);
	thunder::opt_transform(seed_to_angle);

	thunder::LinkageUnit unit;
	unit.add(seed_to_angle);
	unit.add(seed_to_vector);

	fmt::println("{}", unit.generate_glsl());
}