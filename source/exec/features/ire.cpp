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
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

// TODO: algebraic types for shader programming
template <generic T>
struct uplift_base {
	using type = T;
};

template <native T>
struct uplift_base <T> {
	using type = native_t <T>;
};

template <generic T>
struct optional {
	boolean status;
	uplift_base <T> ::type data;
public:
	optional() : status(false) {}

	boolean has_value() {
		return status;
	}

	auto value() {
		return data;
	}

	auto layout() const {
		return uniform_layout("optional",
			named_field(status),
			named_field(data));
	}
};

// TODO: tagged union
template <generic ... Args>
struct variant {};

static_assert(aggregate <optional <f32>>);

// TODO: need to fix nested structures...
struct custom {
	f32 x;
	vec2 y;
	mat4 z;

	auto layout() const {
		return uniform_layout("custom",
			named_field(x),
			named_field(y),
			named_field(z));	
	}
};

auto lambda = [](optional <custom> v)
{
	// TODO: type checking for returns...
	cond(v.has_value());
		returns(v.value().x);
	end();

	returns(0.8f);
};

using S = detail::signature <decltype(lambda)>;
using Result = S::return_t;
using Arguments = S::args_t;
using Procedure = S::manual_prodecure <f32>;

auto ftn = procedure <f32> ("ftn") << lambda;

int main()
{
	thunder::opt_transform(ftn);
	ftn.dump();

	fmt::println("{}", link(ftn).generate_glsl());
}