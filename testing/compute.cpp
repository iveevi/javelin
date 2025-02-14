#include <gtest/gtest.h>

#include <ire/core.hpp>

#include "gl.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(compute);

template <generic T>
using Buffer = write_only_buffer <unsized_array <T>>;

template <generic T, void (*f)(Buffer <T>)>
Procedure <void> kernel = procedure <void> ("main") << []()
{
	local_size(1);

	Buffer <T> buffer(0);

	f(buffer);
};

bestd::optional <GLuint> compile_kernel(const std::string &source)
{
	static int success;
	static char log[512];

	if (!init_context())
		return std::nullopt;

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	const char *source_c_str = source.c_str();
	glShaderSource(shader, 1, &source_c_str, nullptr);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, log);
		JVL_ERROR("Shader compilation error:\n{}", log);
		return std::nullopt;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, shader);
	glLinkProgram(program);
	glDeleteShader(shader);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, log);
		JVL_ERROR("Program linking error:\n{}", log);
		return std::nullopt;
	}

	return program;
}

// TODO: use vulkan...
bool compare(const std::string &reference, Procedure <void> &kernel)
{
	if (!init_context())
		return false;

	// Reference program
	auto opt_prog_ref = compile_kernel(reference);
	if (!opt_prog_ref)
		return false;

	// Javelin generated program
	auto kernel_shader = link(kernel).generate_glsl();

	auto opt_prog_jvl = compile_kernel(kernel_shader);
	if (!opt_prog_jvl)
		return false;

	GLuint prog_ref = opt_prog_ref.value();
	GLuint prog_jvl = opt_prog_jvl.value();

	// TODO: template
	constexpr size_t SIZE = 64;

	std::vector <uint32_t> data_ref(SIZE, 0);
	std::vector <uint32_t> data_jvl(SIZE, 0);

	using Pair = std::pair <GLuint, std::vector <uint32_t> *>;

	// Allocate buffers
	for (auto [program, data] : {
			Pair(prog_ref, &data_ref),
			Pair(prog_jvl, &data_jvl)
		}) {
		GLuint buffer;

		glCreateBuffers(1, &buffer);
		glNamedBufferStorage(buffer, data->size() * sizeof(uint32_t), data->data(), GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

		glUseProgram(program);
		glDispatchCompute(data->size(), 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		glGetNamedBufferSubData(buffer, 0, data->size() * sizeof(uint32_t), data->data());
	}

	glDeleteProgram(prog_ref);
	glDeleteProgram(prog_jvl);

	// Compare
	for (size_t i = 0; i < SIZE; i++) {
		if (data_jvl[i] != data_ref[i]) {
			JVL_ERROR("buffer discrepancy @{}: {} vs {}", i, data_jvl[i], data_ref[i]);
			return false;
		}
	}

	return true;
}

// Native types
void write_native(Buffer <u32> buffer)
{
	buffer[gl_GlobalInvocationID.x] = gl_GlobalInvocationID.x;
}

TEST(compute, native)
{
	std::string reference = R"(
	#version 460

	layout (local_size_x = 1) in;

	layout (binding = 0) buffer Buffer
	{
		uint data[];
	};

	void main()
	{
		data[gl_GlobalInvocationID.x] = gl_GlobalInvocationID.x;
	}
	)";

	ASSERT_TRUE(compare(reference, kernel <u32, write_native>));
}