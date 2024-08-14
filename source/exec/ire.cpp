#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include <fmt/format.h>

#include "atom/atom.hpp"
#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/intrinsics/glsl.hpp"
#include "ire/tagged.hpp"
#include "ire/type_synthesis.hpp"
#include "ire/uniform_layout.hpp"
#include "ire/util.hpp"
#include "ire/vector.hpp"
#include "profiles/targets.hpp"

// TODO: synthesizable with name hints
// TODO: std.hpp for additional features

using namespace jvl::ire;

struct lighting {
	vec3 direction;
	vec3 color;
	boolean on;

	auto layout() {
		// TODO: prevent duplicate fields
		// return uniform_layout(direction, direction);
		return uniform_layout(direction, color, on);
	}
};

// TODO: nested structures
struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;

	lighting light;

	auto layout() {
		return uniform_layout(model, view, light, proj);
		// return uniform_layout(model, view, proj);
	}
};

inline void discard()
{
	platform_intrinsic_keyword("discard");
}

template <typename R, typename ... Args>
struct callable : Callable {
	bool cached;

	std::tuple <Args...> mimic;
	std::tuple <Args *...> passed;

	template <typename U, typename ... UArgs>
	std::tuple <U *, UArgs *...> __fill_parameter_references(U &u, UArgs &... uargs) {
		std::tuple <U *> x = &u;
		if constexpr (sizeof...(UArgs)) {
			std::tuple <UArgs *...> xargs = __fill_parameter_references(uargs...);
			return std::tuple_cat(x, xargs);
		} else {
			return x;
		}
	}

	template <size_t index>
	void __fill_mimic_references() {
		auto &x = std::get <index> (mimic);

		auto &em = Emitter::active;

		using namespace jvl;

		// TODO: parameter global qualifier
		using type_of_x = std::decay_t <decltype(x)>;

		atom::Global global;
		global.qualifier = atom::Global::layout_in;
		global.type = type_field_from_args <type_of_x> ().id;
		global.binding = index;

		x.ref = em.emit(global);

		if constexpr (index + 1 < sizeof...(Args))
			__fill_mimic_references <index + 1> ();
	}

	template <size_t index>
	void __transfer_mimic_references() {
		auto src = std::get <index> (mimic);
		auto dst = std::get <index> (passed);
		dst->ref = src.ref;

		if constexpr (index + 1 < sizeof...(Args))
			__transfer_mimic_references <index + 1> ();
	}

	void call(Args &... active_args) {
		passed = __fill_parameter_references(active_args...);

		if (cached) {
			// TODO: if we here then synthesize a call
			fmt::println("early return, cached");
			return;
		}

		Emitter::active.scopes.push(*this);
		__fill_mimic_references <0> ();
		__transfer_mimic_references <0> ();
	}

	auto &end() {
		// TODO: make sure its at the top
		Emitter::active.scopes.pop();
		return *this;
	}

	operator R() const {
		// TODO: make sure its no longer in the scopes
		return R();
	}
};

// Smith shadow-masking function
// f32 G1(vec3 n, vec3 v, f32 roughness)
// {
// 	// if (dot(v, n) <= 0.0f)
// 	// 	return 0.0f;
//
// 	f32 alpha = roughness;
// 	f32 theta = acos(clamp(dot(n, v), 0, 0.999f));
//
// 	f32 tan_theta = tan(theta);
//
// 	f32 denom = 1 + sqrt(1 + alpha * alpha * tan_theta * tan_theta);
// 	return 2.0f/denom;
// }

auto G1(vec3 n, vec3 v, f32 roughness)
{
	static callable <f32, vec3, vec3, f32> f;

	f.call(n, v, roughness);
	if (f.cached)
		return f;

	f32 alpha = roughness;
	f32 theta = acos(clamp(dot(n, v), 0, 0.999f));

	f32 tan_theta = tan(theta);

	f32 denom = 1 + sqrt(1 + alpha * alpha * tan_theta * tan_theta);
	returns(2.0f/denom);

	// auto f = callable <f32> (n, v, roughness);
	// TODO: Caching callables...

	auto k = f.export_to_kernel();
	k.dump();
	fmt::println("{}", k.synthesize(jvl::profiles::opengl_330));

	return f.end();
}

void vertex_shader()
{
	// TODO: autodiff on inputs...
	// TODO: composing functions
	// TODO: callable functions (cached...)

	layout_in <vec3, 0> position;
	layout_in <vec3, 1> normal;

	layout_out <f32, 0> result;

	result = G1(normal, position, 0.1);

	// push_constant <mvp> mvp;
	//
	// vec4 v = vec4(position, 1);
	// gl_Position = mvp.proj * (mvp.view * (mvp.model * v));
	// gl_Position.y = -gl_Position.y;

	// TODO: immutability for shader inputs
	// TODO: before synthesis, demote variables to inline if they are not modified later
	// TODO: warnings for the unused sections
}

void fragment_shader()
{
	layout_in <vec3, 0> position;
	layout_in <float, 1> depth;

	layout_out <vec4, 0> normal;
	layout_out <vec4, 1> color;

	push_constant <float> tint;

	// TODO: some intrinsics only work on layout ins...
	vec3 dU = dFdx(position);
	vec3 dV = dFdy(position);
	vec3 N = normalize(cross(dU, dV));
	normal = vec4(0.5f + 0.5f * N, 1);
	color = vec4(vec3(depth, depth, depth), tint);
}

// TODO: test on shader toy shaders

#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	auto vertex_kernel = kernel_from_args(vertex_shader);
	std::string vertex_source = vertex_kernel.synthesize(jvl::profiles::opengl_330);
	fmt::println("vsource:\n{}", vertex_source);

	vertex_kernel.dump();

	// auto fragment_kernel = kernel_from_args(fragment_shader);
	// std::string fragment_source = fragment_kernel.synthesize(jvl::profiles::cplusplus_11);
	// fmt::println("fsource:\n{}", fragment_source);

	// glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	// GLFWwindow *window = glfwCreateWindow(800, 800, "Window", NULL, NULL);
	// if (window == NULL) {
	// 	printf("failed to load glfw\n");
	// 	glfwTerminate();
	// 	return -1;
	// }
	// glfwMakeContextCurrent(window);
	//
	// if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
	// 	printf("failed to load glad\n");
	// 	return -1;
	// }
	//
	// GLuint program = glCreateShader(GL_VERTEX_SHADER);
	// const char *source_c_str = vsource.c_str();
	// glShaderSource(program, 1, &source_c_str, nullptr);
	// glCompileShader(program);
	//
	// printf("program: %d\n", program);
	//
	// int success;
	// char error_log[512];
	// glGetShaderiv(program, GL_COMPILE_STATUS, &success);
	// if (!success) {
	// 	glGetShaderInfoLog(program, 512, NULL, error_log);
	// 	fmt::println("compilation error: {}", error_log);
	// }
}
