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

using namespace jvl;
using namespace tsg;

// Full function compilation flow
template <typename F>
auto compile_function(const std::string &name, const F &ftn)
{
	// Light-weight semantic checking
	using T = decltype(function_breakdown(ftn));
	using S = signature <typename T::base>;
	using R = typename S::returns;
	using Args = typename S::arguments;

	// TODO: skip code gen if this fails
	constexpr auto status = classifier <Args> ::status();

	verify_classification <status> check;

	// Secondary pass given the shader stage
	// TODO: pass shader stage as template parameter
	constexpr auto flags = classifier <Args> ::resolved;

	using lifted_results = lift_result <R> ::type;
	using lifted_args = lift_argument <Args> ::type;
	using dependencies = collect_layout_inputs <flags, lifted_args> ::type;
	using results = collect_layout_outputs<flags, lifted_results> ::type;

	// Begin code generation stage
	auto buffer = compiled_artifact_assembler <flags, dependencies, results> ::make();
	buffer.name = name;

	auto &em = ire::Emitter::active;
	em.push(buffer);

	lifted_args arguments;
	shader_args_initializer(arguments);
	auto result = std::apply(ftn, arguments);
	exporting(result);

	em.pop();

	return buffer;
}

// Filtering stage requirements (transformed)
template <ShaderStageFlags	RequiredBy,
	  generic		Type,
	  ShaderStageFlags	RequiredFrom,
	  size_t		Binding>
struct RequirementOut {};

struct SatisfiedRequirement {};

struct NullRequirement {};

template <typename T>
concept requirement = is_stage_meta_v <RequirementOut, T>
	|| std::is_same_v <T, SatisfiedRequirement>
	|| std::is_same_v <T, NullRequirement>;

template <requirement ... Requirements>
struct RequirementVector {
	using filtered = RequirementVector <Requirements...>;

	template <requirement R>
	using inserted = RequirementVector <R, Requirements...>;
};

template <requirement R, requirement ... Requirements>
struct RequirementVector <R, Requirements...> {
	using previous = RequirementVector <Requirements...> ::filtered;
	using filtered = previous::template inserted <R>;

	template <requirement Rs>
	using inserted = RequirementVector <Rs, R, Requirements...>;
};

template <requirement ... Requirements>
struct RequirementVector <NullRequirement, Requirements...> {
	using filtered = RequirementVector <Requirements...> ::filtered;

	template <requirement R>
	using inserted = RequirementVector <R, NullRequirement, Requirements...>;
};

template <specifier S>
struct requirement_for {
	using type = NullRequirement;
};

template <generic T, size_t N>
struct requirement_for <LayoutIn <ShaderStageFlags::eFragment, T, N>> {
	using type = RequirementOut <ShaderStageFlags::eFragment, T,
				     ShaderStageFlags::eVertex, N>;
};

// Requirement satisfiability check
template <requirement R, stage_output ... Outputs>
struct satisfied {
	using result = R;
};

template <requirement R, stage_output O, stage_output ... Outputs>
struct satisfied <R, O, Outputs...> {
	using result = satisfied <R, Outputs...> ::result;
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N, stage_output ... Outputs>
struct satisfied <RequirementOut <S, T, F, N>, StageOut <F, T, N>, Outputs...> {
	using result = SatisfiedRequirement;
};

// TODO: replace with decltype
template <typename Rs, typename Os>
struct satisfied_mega {};

template <requirement ... Requirements, stage_output ... Outputs>
struct satisfied_mega <RequirementVector <Requirements...>, StageOutputs <Outputs...>> {
	using result = RequirementVector <typename satisfied <Requirements, Outputs...> ::result...>;
};

// Error stage for requirements
// TODO: array based approach?
template <requirement ... Requirements>
struct requirement_error_report {};

template <requirement R, requirement ... Requirements>
struct requirement_error_report <R, Requirements...> {
	static constexpr auto next = requirement_error_report <Requirements...>();
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N>
struct dedicated_reporter {
	static_assert(false, "shader dependency is unsatisfied");
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N, requirement ... Requirements>
struct requirement_error_report <RequirementOut <S, T, F, N>, Requirements...> {
	static constexpr auto next = requirement_error_report <Requirements...> ();
	static constexpr auto reporter = dedicated_reporter <S, T, F, N> ();
};

template <typename T>
struct requirement_error_report_vector {};

template <requirement ... Requirements>
struct requirement_error_report_vector <RequirementVector <Requirements...>> {
	using type = requirement_error_report <Requirements...>;
};

// Status
template <requirement R>
struct requirement_status {
	static constexpr bool status = true;
};

template <ShaderStageFlags S, generic T, ShaderStageFlags F, size_t N>
struct requirement_status <RequirementOut <S, T, F, N>> {
	static constexpr bool status = false;
};

template <requirement R, requirement ... Requirements>
constexpr bool verification_status()
{
	using reporter = requirement_status <R>;
	if constexpr (sizeof...(Requirements))
		return reporter::status && verification_status <Requirements...> ();

	return reporter::status;
}

template <requirement ... Requirements>
constexpr bool verification_status(const RequirementVector <Requirements...> &reqs)
{
	if constexpr (sizeof...(Requirements))
		return verification_status <Requirements...> ();

	return true;
}

// Shader linkage unit
template <ShaderStageFlags flags = ShaderStageFlags::eNone, typename ... Args>
struct ShaderProgram {};

// From zero
template <>
struct ShaderProgram <ShaderStageFlags::eNone> {
	template <ShaderStageFlags added, specifier ... Specifiers>
	friend auto operator<<(const ShaderProgram &program,
			       const CompiledArtifact <added, Specifiers...> &compiled) {
		ShaderProgram <added, Specifiers...> result {
			static_cast <thunder::TrackedBuffer> (compiled)
		};

		return result;
	}
};

// TODO: generalize order, with another structure to specify compatibility
template <specifier ... Specifiers>
struct ShaderProgram <ShaderStageFlags::eVertex, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;

	template <specifier ... AddedSpecifiers>
	friend auto operator<<(const ShaderProgram &program,
			       const CompiledArtifact <ShaderStageFlags::eFragment, AddedSpecifiers...> &compiled) {
		ShaderProgram <ShaderStageFlags::eVxF,
			       Specifiers...,
			       AddedSpecifiers...> result {
			program.ir_vertex,
			static_cast <thunder::TrackedBuffer> (compiled)
		};

		return result;
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
	using satisfied = typename satisfied_mega <requirements, outputs> ::result;

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
