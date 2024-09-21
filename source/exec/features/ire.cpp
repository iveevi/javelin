// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

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
	u32 x = (s.a << s.b) ^ (s.b - s.a);
	u32 y = (s.b + s.a * s.b)/s.a;
	return uvec2(x, y);
};
int main()
{
	thunder::opt_transform(seed_to_vector);

	thunder::LinkageUnit unit;
	// unit.add(seed_to_angle);
	unit.add(seed_to_vector);

	fmt::println("{}", unit.generate_cpp());

	auto ftn = unit.jit();

	auto f = jit(seed_to_vector);

	auto s = solid_t <seed> ();
	s.get <0> () = 1;
	s.get <1> () = 5;

	auto v = f(s);

	fmt::println("v: ({}, {})", v.x, v.y);
}