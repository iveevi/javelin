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

auto seed_to_vector = callable("seed_to_vector")
	<< std::make_tuple(seed(), 16)
	<< [](const seed &s, int bias)
{
	push_constant <uint32_t> pc(16);
	uniform <uint32_t> prime(1);
	shared <uvec2> offset1;
	shared <uvec2> offset2;

	u32 x = (s.a << s.b) ^ (s.b - s.a);

	auto iterations = range <f32> (0, 10.0, 1.618);

	auto iter = loop(iterations);
		u32 y = floatBitsToUint(bias * iter) + (s.b + s.a * s.b)/s.a;
		x += y - pc + dot(offset1, 1 - offset2);

		cond((x * prime) % 2 == 1);
			stop();
		end();
	end();

	returns(uvec2(x, y));
};

int main()
{
	thunder::opt_transform(seed_to_vector);
	auto unit = link(seed_to_vector, seed_to_vector);
	fmt::println("{}", unit.generate_glsl());
}