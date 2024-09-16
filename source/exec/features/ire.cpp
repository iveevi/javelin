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

using namespace jvl;
using namespace jvl::ire;
	
MODULE(ire-features);

struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj)
		);
	}
};

void vertex()
{
	layout_in <vec3> position(0);
	layout_out <vec3> out_position(0);

	gl_Position = vec4(position, 1);
	out_position = position;
}

void fragment()
{
	struct shading_state {
		vec3 position;
		vec3 normal;
		array <vec3, 4> samples;

		vec3 sample_average() const {
			vec3 sum;
			for (uint32_t i = 0; i < samples.size(); i++)
				sum += samples[i];
			return sum / f32(samples.size());
		}
	};

	layout_in <vec3> position(0);
	layout_out <vec4> fragment(0);

	shading_state state;
	state.position = position;
	
	fragment = vec4(state.sample_average(), 1);
}

GLuint compile_glsl_source(std::string &source, GLuint stage)
{
	GLuint program = glCreateShader(stage);
	const char *source_c_str = source.c_str();
	glShaderSource(program, 1, &source_c_str, nullptr);
	glCompileShader(program);

	int success;
	char log[512];

	glGetShaderiv(program, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(program, 512, nullptr, log);
		JVL_ERROR("compilation error:\n{}", log);
	}

	return program;
}

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(800, 800, "JVL:TEST", NULL, NULL);
	if (window == NULL) {
		JVL_ERROR("failed to load glfw");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
		JVL_ERROR("failed to load glad");
		return -1;
	}

	auto vs = kernel_from_args(vertex).compile(profiles::glsl_450);
	auto fs = kernel_from_args(fragment).compile(profiles::glsl_450);

	fmt::println("{}", vs);
	fmt::println("{}", fs);
	
	auto vs_program = compile_glsl_source(vs, GL_VERTEX_SHADER);
	auto fs_program = compile_glsl_source(fs, GL_FRAGMENT_SHADER);

	GLuint linked = glCreateProgram();
	glAttachShader(linked, vs_program);
	glAttachShader(linked, fs_program);

	glLinkProgram(linked);
	
	int success;
	char log[512];

	glGetProgramiv(linked, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(linked, 512, nullptr, log);
		JVL_ERROR("linking error:\n{}", log);
	}
}