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

template <builtin T>
struct range {
	T start;
	T end;
	T step;

	range(const T &start_, const T &end_, const T &step_ = T(1))
		: start(start_), end(end_), step(step_) {}
};

template <builtin T>
inline T loop(const range <T>  &range)
{
	auto &em = Emitter::active;

	T i = range.start;
	boolean cond = i < range.end;
	
	auto pre = [i, range](){
		T l = i;
		l += range.step;
	};

	em.emit_branch(cond.synthesize().id, -1, thunder::loop_while, pre);
	return i;
}

auto seed_to_vector = callable("seed_to_vector")
	<< std::make_tuple(seed(), 16)
	<< [](const seed &s, int bias)
{
	u32 x = (s.a << s.b) ^ (s.b - s.a);

	auto iterations = range <f32> (0, 10.0, 1.618);

	auto iter = loop(iterations);
		u32 y = floatBitsToUint(bias * iter) + (s.b + s.a * s.b)/s.a;
		x += y;
	end();

	returns(uvec2(x, y));
};

int main()
{
	thunder::opt_transform(seed_to_vector);
	auto unit = link(seed_to_vector, seed_to_vector);
	fmt::println("{}", unit.generate_glsl());
}