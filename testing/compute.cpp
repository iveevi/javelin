#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <gtest/gtest.h>

#include <ire.hpp>

#include "gl.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(compute);

namespace glm {

std::string format_as(const uvec3 &v)
{
	return to_string(v);
}

}

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
	static std::string log(512, '\0');

	if (!init_context())
		return std::nullopt;

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	const char *source_c_str = source.c_str();
	glShaderSource(shader, 1, &source_c_str, nullptr);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, log.size(), NULL, log.data());
		JVL_ERROR("Shader compilation error:\n{}", log);
		return std::nullopt;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, shader);
	glLinkProgram(program);
	glDeleteShader(shader);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, log.size(), NULL, log.data());
		JVL_ERROR("Program linking error:\n{}", log);
		return std::nullopt;
	}

	return program;
}

struct Datum {
	f32 x;
	i32 y;
	u32 z;

	auto layout() {
		return layout_from("Datum",
			verbatim_field(x),
			verbatim_field(y),
			verbatim_field(z));
	}
};

bool operator!=(const solid_t <Datum> &a, const solid_t <Datum> &b)
{
	return (a.get <0> () != b.get <0> ())
		|| (a.get <1> () != b.get <1> ())
		|| (a.get <2> () != b.get <2> ());
}

namespace jvl::ire {

std::string format_as(const solid_t <Datum> &a)
{
	return fmt::format("Datum(x: {:.3f}, y: {}, z: {})",
		a.get <0> (),
		a.get <1> (),
		a.get <2> ());
}

}

template <generic T, void (*f)(Buffer <T>)>
bool compare(const std::string &reference)
{
	if (!init_context())
		return false;

	// Reference program
	auto opt_prog_ref = compile_kernel(reference);
	if (!opt_prog_ref)
		return false;

	// Javelin generated program
	auto kernel_shader = link(kernel <T, f>).generate_glsl();

	auto opt_prog_jvl = compile_kernel(kernel_shader);
	if (!opt_prog_jvl) {
		fmt::println("{}", kernel_shader);
		return false;
	}

	GLuint prog_ref = opt_prog_ref.value();
	GLuint prog_jvl = opt_prog_jvl.value();

	// TODO: template
	constexpr size_t SIZE = 64;

	using S = solid_t <T>;
	std::vector <S> data_ref(SIZE, S());
	std::vector <S> data_jvl(SIZE, S());

	using Pair = std::pair <GLuint, std::vector <S> *>;

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
			fmt::println("{}", kernel_shader);
			return false;
		}
	}

	return true;
}

//////////////////
// Native types //
//////////////////

void write_native(Buffer <u32> buffer)
{
	u32 x = gl_GlobalInvocationID.x;
	buffer[x] = x;
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

	auto status = compare <u32, write_native> (reference);
	
	ASSERT_TRUE(status);
}

///////////////////
// Builtin types //
///////////////////

void write_builtin(Buffer <uvec3> buffer)
{
	u32 x = gl_GlobalInvocationID.x;
	buffer[x] = uvec3(3 * x, 3 * x + 1, 3 * x + 2);
}

TEST(compute, builtin)
{
	std::string reference = R"(
	#version 460

	layout (local_size_x = 1) in;

	layout (binding = 0) buffer Buffer
	{
		uvec3 data[];
	};

	void main()
	{
		uint x = gl_GlobalInvocationID.x;
		data[x] = uvec3(3 * x, 3 * x + 1, 3 * x + 2);
	}
	)";

	auto status = compare <uvec3, write_builtin> (reference);
	
	ASSERT_TRUE(status);
}

/////////////////////
// Aggregate types //
/////////////////////

void write_aggregate(Buffer <Datum> buffer)
{
	u32 x = gl_GlobalInvocationID.x;
	buffer[x] =  Datum(uintBitsToFloat(x), -i32(x), x);
}

TEST(compute, aggregate)
{
	std::string reference = R"(
	#version 460

	layout (local_size_x = 1) in;

	struct Datum {
		float x;
		int y;
		uint z;
	};

	layout (binding = 0) buffer Buffer
	{
		Datum data[];
	};

	void main()
	{
		uint x = gl_GlobalInvocationID.x;
		data[x] = Datum(uintBitsToFloat(x), -int(x), x);
	}
	)";

	auto status = compare <Datum, write_aggregate> (reference);
	
	ASSERT_TRUE(status);
}