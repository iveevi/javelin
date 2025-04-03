#include "util.hpp"

#include <ire.hpp>

#include <common/io.hpp>

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
	$subroutine(i32, sum, i32 x, i32 y) {
		$return x + y;
	};

	auto glsl = link(sum).generate_glsl();
	
	io::display_lines("SUM", glsl);
	
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
	$subroutine(f32, arithmetic, f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		f32 b = a / (x - y) * z * z;
		$return a / b;
	};

	auto glsl = link(arithmetic).generate_glsl();
	
	io::display_lines("ARITHMETIC", glsl);

	check_shader_sources(expected_arithmetic_glsl, glsl);
}

TEST(callable, returns)
{
	$subroutine(f32, arithmetic, f32 x, f32 y, f32 z) {
		f32 a = x + y * z;
		f32 b = a / (x - y) * z * z;
		$return a / b;
	};

	auto glsl = link(arithmetic).generate_glsl();
	
	io::display_lines("RETURNS", glsl);

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
	$subroutine(f32, conditional, f32 x, f32 y, f32 z) {
		f32 a = x + y * z;

		$if (a < 0) {
			f32 b = a / (x - y) * z * z;
			$return a / b;
		};

		$return a;
	};

	auto glsl = link(conditional).generate_glsl();
	
	io::display_lines("CONDITIONAL RETURNS", glsl);

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
    vec4 s0 = vec4(vec3(_arg1), 1);
    s0 = (_arg0.model * s0);
    s0 = (_arg0.view * s0);
    s0 = (_arg0.proj * s0);
    return vec4(s0);
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

	$subroutine(vec4, project, MVP mvp, vec3 v) {
		vec4 vh = vec4(v, 1);
		vh = mvp.model * vh;
		vh = mvp.view * vh;
		vh = mvp.proj * vh;
		$return vh;
	};

	auto glsl = link(project).generate_glsl();

	io::display_lines("STRUCT PARAMETER", glsl);

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
    return Seed(((_arg0.root << _arg0.shifted) & (_arg0.shifted | _arg0.root)), (_arg0.shifted | _arg0.root));
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

	$subroutine(Seed, shift_seed, Seed seed) {
		u32 a = seed.root << seed.shifted;
		u32 b = seed.shifted | seed.root;
		$return Seed(a & b, b);
	};

	auto glsl = link(shift_seed).generate_glsl();
	
	io::display_lines("STRUCT RETURN", glsl);

	check_shader_sources(expected_struct_return_glsl, glsl);
}