#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/classify.hpp>
#include <tsg/exporting.hpp>
#include <tsg/flags.hpp>
#include <tsg/imports.hpp>
#include <tsg/initializer.hpp>
#include <tsg/intrinsics.hpp>
#include <tsg/lifting.hpp>
#include <tsg/meta.hpp>
#include <tsg/signature.hpp>
#include <tsg/layouts.hpp>
#include <tsg/artifact.hpp>
#include <tsg/outputs.hpp>
#include <tsg/requirement.hpp>
#include <tsg/compile.hpp>
#include <tsg/resolution.hpp>
#include <tsg/error.hpp>

using namespace jvl;
using namespace tsg;

// Shader linkage unit
template <ShaderStageFlags flags = ShaderStageFlags::eNone, typename ... Args>
struct ShaderProgram {};

// From zero
template <>
struct ShaderProgram <ShaderStageFlags::eNone> {
	template <ShaderStageFlags added, specifier ... Specifiers>
	friend auto operator<<(const ShaderProgram &program,
			       const CompiledArtifact <added, Specifiers...> &compiled) {
		return ShaderProgram <added, Specifiers...> {
			static_cast <thunder::TrackedBuffer> (compiled)
		};
	}
};

// TODO: generalize order, with another structure to specify compatibility
template <specifier ... Specifiers>
struct ShaderProgram <ShaderStageFlags::eVertex, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;

	template <specifier ... AddedSpecifiers>
	friend auto operator<<(const ShaderProgram &program,
			       const CompiledArtifact <ShaderStageFlags::eFragment, AddedSpecifiers...> &compiled) {
		return ShaderProgram <ShaderStageFlags::eVxF, Specifiers..., AddedSpecifiers...> {
			program.ir_vertex,
			static_cast <thunder::TrackedBuffer> (compiled)
		};
	}
};

template <specifier ... Specifiers>
struct ShaderProgram <ShaderStageFlags::eVxF, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;
	thunder::TrackedBuffer ir_fragment;

	// Layout verification
	using raw_requirements = RequirementVector <typename requirement_for <Specifiers> ::type...>;
	using requirements = typename raw_requirements::filtered;
	using outputs = typename collector <StageOutputs <>, Specifiers...> ::result;
	using satisfied = typename resolve_requirement_vector <requirements, outputs> ::result;

	static constexpr auto errors = typename requirement_error_report_vector <satisfied> ::type();
	static constexpr bool valid = verification_status(satisfied());

	// TODO: extract descriptor set layout
};

// Shader pipeline
struct Pipline {

};

// Testing
auto vertex_shader(vertex_intrinsics, const vec3 pos) -> std::tuple <position, vec3, vec2>
{
	vec3 p = pos;
	cond(p.x > 0.0f);
		p.y = 1.0f;
	end();

	return std::make_tuple(vec4(p, 1), vec3(), vec2());
}

// TODO: deprecation warnings on unused layouts
auto fragment_shader(fragment_intrinsics, vec3 pos, vec2 normal) -> vec4
{
	return vec4(1, 0, 0, 0);
}

int main()
{
	auto vs = compile_function("main", vertex_shader);
	auto fs = compile_function("main", fragment_shader);

	auto unit = ShaderProgram();
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
