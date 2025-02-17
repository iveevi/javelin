#include "util.hpp"

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

const std::string expected_sum_glsl = R"(
#version 460

int sum(int _arg0, int _arg1)
{
    return (_arg0 + _arg1);
}
)";

TEST(callable, sum)
{
	auto ftn = [](i32 x, i32 y) {
		return x + y;
	};

	auto F = procedure("sum") << ftn;
	auto glsl = link(F).generate_glsl();

	// fmt::println("{}", glsl);

	check_shader_sources(expected_sum_glsl, glsl);
}

const std::string expected_arithmetic_glsl = R"(
#version 460

float arithmetic(float _arg0, float _arg1, float _arg2)
{
    return ((_arg0 + (_arg1 * _arg2)) / ((((_arg0 + (_arg1 * _arg2)) / (_arg0 - _arg1)) * _arg2) * _arg2));
}
)";

TEST(callable, arithmetic)
{
	auto ftn = [](f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		f32 b = a / (x - y) * z * z;
		return a / b;
	};

	auto F = procedure("arithmetic") << ftn;
	auto glsl = link(F).generate_glsl();
	
	// fmt::println("{}", glsl);

	check_shader_sources(expected_arithmetic_glsl, glsl);
}

TEST(callable, returns)
{
	auto ftn = [](f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		f32 b = a / (x - y) * z * z;
		returns(a / b);
	};

	auto F = procedure <f32> ("arithmetic") << ftn;
	auto glsl = link(F).generate_glsl();
	
	// fmt::println("{}", glsl);

	check_shader_sources(expected_arithmetic_glsl, glsl);
}

// TODO: partial evaluation
const std::string expected_conditional_returns_glsl = R"(
#version 460

float conditional(float _arg0, float _arg1, float _arg2)
{
    if (((_arg0 + (_arg1 * _arg2)) < 0)) {
        return ((_arg0 + (_arg1 * _arg2)) / ((((_arg0 + (_arg1 * _arg2)) / (_arg0 - _arg1)) * _arg2) * _arg2));
    }
    return (_arg0 + (_arg1 * _arg2));
}
)";

TEST(callable, conditional_returns)
{
	auto ftn = [](f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		cond(a < 0);
		{
			f32 b = a / (x - y) * z * z;
			returns(a / b);
		}
		end();

		return a;
	};

	auto cbl = procedure("conditional") << ftn;
	auto glsl = link(cbl).generate_glsl();

	check_shader_sources(expected_conditional_returns_glsl, glsl);
}

const std::string expected_struct_parameter_glsl = R"(
#version 460

struct MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
};

vec4 project(MVP _arg0, vec3 _arg1)
{
    vec4 s0 = vec4(_arg1, 1);
    s0 = (_arg0.model * s0);
    s0 = (_arg0.view * s0);
    s0 = (_arg0.proj * s0);
    return s0;
}
)";

TEST(callable, struct_parameter)
{
	struct MVP {
		mat4 model;
		mat4 view;
		mat4 proj;

		auto layout() {
			return layout_from("MVP",
				verbatim_field(model),
				verbatim_field(view),
				verbatim_field(proj));
		}
	};

	auto ftn = [](const MVP &mvp, const vec3 &v) {
		vec4 vh = vec4(v, 1);
		vh = mvp.model * vh;
		vh = mvp.view * vh;
		vh = mvp.proj * vh;
		return vh;
	};

	auto F = procedure("project") << ftn;
	auto glsl = link(F).generate_glsl();

	// fmt::println("{}", glsl);

	check_shader_sources(expected_struct_parameter_glsl, glsl);
}

const std::string expected_struct_return_glsl = R"(
#version 460

struct Seed {
    uint root;
    uint shifted;
};

Seed shift_seed(Seed _arg0)
{
    Seed s0 = Seed(((_arg0.root << _arg0.shifted) & (_arg0.shifted | _arg0.root)), (_arg0.shifted | _arg0.root));
    return s0;
}
)";

TEST(callable, struct_return)
{
	struct Seed {
		u32 root;
		u32 shifted;

		auto layout() {
			return layout_from("Seed",
				verbatim_field(root),
				verbatim_field(shifted));
		}
	};

	auto ftn = [](Seed seed) {
		u32 a = seed.root << seed.shifted;
		u32 b = seed.shifted | seed.root;
		return Seed(a & b, b);
	};

	auto F = procedure("shift_seed") << ftn;
	auto glsl = link(F).generate_glsl();
	
	// fmt::println("{}", glsl);

	check_shader_sources(expected_struct_return_glsl, glsl);
}