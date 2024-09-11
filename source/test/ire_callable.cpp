#include "util.hpp"

#include "ire/core.hpp"
#include "profiles/targets.hpp"

using namespace jvl;
using namespace jvl::ire;

const std::string expected_sum_glsl = R"(
#version 450

int main(int _arg0)
{
    return _arg0;
}
)";

TEST(ire_callable, sum)
{
	auto ftn = [](i32 x) {
		return x;
	};

	auto cbl = callable(ftn).named("sum");
	auto kernel = cbl.export_to_kernel();
	auto glsl = kernel.compile(profiles::glsl_450);

	// fmt::println("{}", glsl);

	check_shader_sources(expected_sum_glsl, glsl);
}

// TODO: refactor to callables' names
const std::string expected_arithmetic_glsl = R"(
#version 450

float main(float _arg0, float _arg1, float _arg2)
{
    return ((_arg0 + (_arg1 * _arg2)) / ((((_arg0 + (_arg1 * _arg2)) / (_arg0 - _arg1)) * _arg2) * _arg2));
}
)";

TEST(ire_callable, arithmetic)
{
	auto ftn = [](f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		f32 b = a / (x - y) * z * z;
		return a / b;
	};

	auto cbl = callable(ftn).named("arithemtic");
	auto kernel = cbl.export_to_kernel();
	auto glsl = kernel.compile(profiles::glsl_450);
	
	// fmt::println("{}", glsl);

	check_shader_sources(expected_arithmetic_glsl, glsl);
}

TEST(ire_callable, returns)
{
	auto ftn = [](f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		f32 b = a / (x - y) * z * z;
		returns(a / b);
	};

	auto cbl = callable <f32> (ftn).named("arithemtic");
	auto kernel = cbl.export_to_kernel();
	auto glsl = kernel.compile(profiles::glsl_450);
	
	// fmt::println("{}", glsl);

	check_shader_sources(expected_arithmetic_glsl, glsl);
}

// TODO: partial evaluation
const std::string expected_conditional_returns_glsl = R"(
#version 450

float main(float _arg0, float _arg1, float _arg2)
{
    if (((_arg0 + (_arg1 * _arg2)) < 0)) {
        return ((_arg0 + (_arg1 * _arg2)) / ((((_arg0 + (_arg1 * _arg2)) / (_arg0 - _arg1)) * _arg2) * _arg2));
    }
    return (_arg0 + (_arg1 * _arg2));
}
)";

TEST(ire_callable, conditional_returns)
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

	auto cbl = callable(ftn).named("conditional");
	auto kernel = cbl.export_to_kernel();
	auto glsl = kernel.compile(profiles::glsl_450);

	check_shader_sources(expected_conditional_returns_glsl, glsl);
}

const std::string expected_struct_parameter_glsl = R"(
#version 450

struct s0_t {
    mat4 f0;
    mat4 f1;
    mat4 f2;
};

vec4 main(s0_t _arg0, vec3 _arg1)
{
    vec4 s0 = vec4(_arg1, 1);
    s0 = (_arg0.f0 * s0);
    s0 = (_arg0.f1 * s0);
    s0 = (_arg0.f2 * s0);
    return s0;
}
)";

TEST(ire_callable, struct_parameter)
{
	struct MVP {
		mat4 model;
		mat4 view;
		mat4 proj;

		auto layout() const {
			return uniform_layout(
				"MVP",
				named_field(model),
				named_field(view),
				named_field(proj)
			);
		}
	};

	auto ftn = [](const MVP &mvp, const vec3 &v) {
		vec4 vh = vec4(v, 1);
		vh = mvp.model * vh;
		vh = mvp.view * vh;
		vh = mvp.proj * vh;
		return vh;
	};

	auto cbl = callable(ftn).named("project");
	auto kernel = cbl.export_to_kernel();
	auto glsl = kernel.compile(profiles::glsl_450);

	// fmt::println("{}", glsl);

	check_shader_sources(expected_struct_parameter_glsl, glsl);
}

const std::string expected_struct_return_glsl = R"(
#version 450

struct s0_t {
    uint f0;
    uint f1;
};

uint main(s0_t _arg0)
{
    return ((_arg0.f0 << _arg0.f1) & (_arg0.f1 | _arg0.f0));
}
)";

TEST(ire_callable, struct_return)
{
	struct Seed {
		u32 root;
		u32 shifted;

		auto layout() const {
			return uniform_layout(
				"Seed",
				named_field(root),
				named_field(shifted)
			);
		}
	};

	auto ftn = [](Seed seed) {
		u32 a = seed.root << seed.shifted;
		u32 b = seed.shifted | seed.root;
		return a & b;
	};

	auto cbl = callable(ftn).named("shift_seed");
	auto kernel = cbl.export_to_kernel();
	auto glsl = kernel.compile(profiles::glsl_450);

	// fmt::println("{}", glsl);

	check_shader_sources(expected_struct_return_glsl, glsl);
}