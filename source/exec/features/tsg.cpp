#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/compile.hpp>
#include <tsg/program.hpp>

using namespace jvl;
using namespace tsg;

// Shader pipeline
struct Pipline {

};

// Projection matrix
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

// Testing
// TODO: check for duplicate push constants
auto vertex_shader(VertexIntrinsics, PushConstant <MVP> mvp, vec3 pos)
{
	vec3 p = pos;
	// cond(p.x > 0.0f);
	// 	p.y = second;
	// end();

	vec4 pp = mvp.project(p);

	return std::make_tuple(Position(pp), vec3(), vec3());
}

// TODO: deprecation warnings on unused layouts
auto fragment_shader(FragmentIntrinsics, vec3 pos)
{
	return vec4(1, 0, 0, 0);
}
	
using T = decltype(function_breakdown(vertex_shader));
using S = signature <typename T::base>;
using R = typename S::returns;
using Args = typename S::arguments;

// Secondary pass given the shader stage
using lifted_results = lift_result <R> ::type;
using lifted_args = lift_argument <Args> ::type;

int main()
{
	auto vs = compile_function("main", vertex_shader);

	// thunder::opt_transform(vs);

	fmt::println("{}", link(vs).generate_glsl());

	// auto fs = compile_function("main", fragment_shader);

	// auto unit = Program();
	// auto next = unit << vs << fs;

	// thunder::opt_transform(next.ir_vertex);
	// next.ir_vertex.dump();
	// auto vunit = thunder::LinkageUnit();
	// vunit.add(next.ir_vertex);
	// fmt::println("{}", vunit.generate_glsl());
	// auto vspirv = vunit.generate_spirv(vk::ShaderStageFlagBits::eVertex);

	// thunder::opt_transform(next.ir_fragment);
	// next.ir_fragment.dump();
	// auto funit = thunder::LinkageUnit();
	// funit.add(next.ir_fragment);
	// fmt::println("{}", funit.generate_glsl());
	// auto fspirv = funit.generate_spirv(vk::ShaderStageFlagBits::eFragment);
}
