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

// Testing
auto vertex_shader(vertex_intrinsics, vec3 pos)
{
	vec3 p = pos;
	cond(p.x > 0.0f);
		p.y = 1.0f;
	end();

	return std::make_tuple(position(p, 1), vec3(), vec3());
}

// TODO: deprecation warnings on unused layouts
auto fragment_shader(fragment_intrinsics, vec3 pos)
{
	return vec4(1, 0, 0, 0);
}

int main()
{
	auto vs = compile_function("main", vertex_shader);
	auto fs = compile_function("main", fragment_shader);

	auto unit = Program();
	auto next = unit << vs << fs;

	thunder::opt_transform(next.ir_vertex);
	next.ir_vertex.dump();
	auto vunit = thunder::LinkageUnit();
	vunit.add(next.ir_vertex);
	fmt::println("{}", vunit.generate_glsl());
	auto vspirv = vunit.generate_spirv(vk::ShaderStageFlagBits::eVertex);

	thunder::opt_transform(next.ir_fragment);
	next.ir_fragment.dump();
	auto funit = thunder::LinkageUnit();
	funit.add(next.ir_fragment);
	fmt::println("{}", funit.generate_glsl());
	auto fspirv = funit.generate_spirv(vk::ShaderStageFlagBits::eFragment);
}
