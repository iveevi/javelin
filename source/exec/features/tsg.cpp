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

using namespace jvl;
using namespace tsg;

// Specifiers for a shader program
template <ShaderStageFlags F, generic T, size_t N>
struct LayoutIn {};

template <ShaderStageFlags F, generic T, size_t N>
struct LayoutOut {};

template <typename T>
concept layout_input = is_stage_meta_v <LayoutIn, T>;

template <typename T>
concept layout_output = is_stage_meta_v <LayoutOut, T>;

// Set of all specifier types
template <typename T>
concept specifier = layout_input <T> || layout_output <T>;

// Collection of the layout input dependencies of a program
template <layout_input ... Args>
struct LayoutInputSet {};

template <layout_input T, typename ... Args>
struct append_layout_input_set {
	using type = LayoutInputSet <>;
};

template <layout_input T, layout_input ... Args>
struct append_layout_input_set <T, LayoutInputSet <Args...>> {
	using type = LayoutInputSet <T, Args...>;
};

// Constructing layout input dependencies
template <ShaderStageFlags F, size_t I, typename ... Args>
struct layout_input_collector {
	using type = LayoutInputSet <>;
};

template <ShaderStageFlags F, size_t I, typename T, typename ... Args>
struct layout_input_collector <F, I, T, Args...> {
	using type = typename layout_input_collector <F, I, Args...> ::type;
};

template <ShaderStageFlags F, size_t I, generic T, typename ... Args>
struct layout_input_collector <F, I, ire::layout_in <T>, Args...> {
	using next = typename layout_input_collector <F, I + 1, Args...> ::type;
	using type = append_layout_input_set <LayoutIn <F, T, I>, next> ::type;
};

template <ShaderStageFlags, typename ... Args>
struct layout_input_collector_args {};

template <ShaderStageFlags F, typename ... Args>
struct layout_input_collector_args <F, std::tuple <Args...>> {
	using base = layout_input_collector <F, 0, Args...>;
	using type = typename base::type;
};

// Collection of the layout outputs of a program
template <layout_output ... Args>
struct LayoutOutputSet {};

template <layout_output T, typename ... Args>
struct append_layout_output_set {
	using type = LayoutOutputSet <>;
};

template <layout_output T, layout_output ... Args>
struct append_layout_output_set <T, LayoutOutputSet <Args...>> {
	using type = LayoutOutputSet <T, Args...>;
};

// Constructing layout input dependencies
template <ShaderStageFlags F, size_t I, typename ... Args>
struct layout_output_collector {
	using type = LayoutOutputSet <>;
};

template <ShaderStageFlags F, size_t I, typename T, typename ... Args>
struct layout_output_collector <F, I, T, Args...> {
	using type = typename layout_output_collector <F, I, Args...> ::type;
};

template <ShaderStageFlags F, size_t I, generic T, typename ... Args>
struct layout_output_collector <F, I, ire::layout_out <T>, Args...> {
	using next = typename layout_output_collector <F, I + 1, Args...> ::type;
	using type = append_layout_output_set <LayoutOut <F, T, I>, next> ::type;
};

template <ShaderStageFlags F, typename ... Args>
struct layout_output_collector_args {
	using base = layout_output_collector <F, 0, Args...>;
	using type = typename base::type;
};

template <ShaderStageFlags F, typename ... Args>
struct layout_output_collector_args <F, std::tuple <Args...>> {
	using base = layout_output_collector <F, 0, Args...>;
	using type = typename base::type;
};

// Type-safe wrapper of a tracked buffer
template <ShaderStageFlags, specifier ... Args>
struct CompiledArtifact : thunder::TrackedBuffer {};

template <ShaderStageFlags, typename ... Args>
struct CompiledArtifactAssembler {
	static_assert(false);
};

// TODO: constrain to layout types...
template <ShaderStageFlags flags, layout_input ... Inputs, layout_output ... Outputs>
struct CompiledArtifactAssembler <flags, LayoutInputSet <Inputs...>, LayoutOutputSet <Outputs...>> {
	static auto get() {
		return CompiledArtifact <flags, Inputs..., Outputs...> ();
	}
};

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

	using LiftedR = typename lift_result <R> ::type;
	using LiftedArgs = typename lift_argument <Args> ::type;
	using Dependencies = typename layout_input_collector_args <flags, LiftedArgs> ::type;
	using Result = typename layout_output_collector_args <flags, LiftedR> ::type;

	// Begin code generation stage
	auto buffer = CompiledArtifactAssembler <flags, Dependencies, Result> ::get();
	buffer.name = name;

	auto &em = ire::Emitter::active;
	em.push(buffer);

	LiftedArgs arguments;
	shader_args_initializer(arguments);
	auto result = std::apply(ftn, arguments);
	exporting(result);

	// TODO: subroutine

	em.pop();

	return buffer;
}

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

// Indexing variadic arguments
template <size_t I, typename ... Args>
struct indexer {};

template <typename T, typename ... Args>
struct indexer <0, T, Args...> {
	using type = T;
};

template <size_t I, typename ... Args>
using type_at = indexer <I, Args...> ::type;

// Filtering stage requirements (transformed)
template <ShaderStageFlags	RequiredBy,
	  generic		Type,
	  ShaderStageFlags	RequiredFrom,
	  size_t		Binding>
struct RequirementOut {};

struct SatisfiedRequirement {};

struct EmptyRequirement {};

template <typename T>
concept requirement = is_stage_meta_v <RequirementOut, T>
	|| std::is_same_v <T, SatisfiedRequirement>
	|| std::is_same_v <T, EmptyRequirement>;

template <requirement ... Requirements>
struct RequirementVector {
	using filtered = RequirementVector <Requirements...>;

	template <requirement R>
	using inserted = RequirementVector <R, Requirements...>;
};

template <requirement R, requirement ... Requirements>
struct RequirementVector <R, Requirements...> {
	using sub_filtered = RequirementVector <Requirements...> ::filtered;
	using filtered = sub_filtered::template inserted <R>;

	template <requirement Rs>
	using inserted = RequirementVector <Rs, R, Requirements...>;
};

template <requirement ... Requirements>
struct RequirementVector <EmptyRequirement, Requirements...> {
	using filtered = RequirementVector <Requirements...> ::filtered;

	template <requirement R>
	using inserted = RequirementVector <R, EmptyRequirement, Requirements...>;
};

template <specifier S>
struct layout_requirement {
	using type = EmptyRequirement;
};

template <generic T, size_t N>
struct layout_requirement <LayoutIn <ShaderStageFlags::eFragment, T, N>> {
	using type = RequirementOut <ShaderStageFlags::eFragment, T,
				     ShaderStageFlags::eVertex, N>;
};

// Filtering out stage outputs
template <ShaderStageFlags F, generic T, size_t N>
struct StageOut {};

// TODO: easier concept creation; e.g. type_accepter <...> but templates could be problematic -- template template parameters?
template <typename T>
struct is_stage_out_type : std::false_type {};

template <ShaderStageFlags F, generic T, size_t N>
struct is_stage_out_type <StageOut <F, T, N>> : std::true_type {};

template <typename T>
concept stage_output = is_stage_out_type <T> ::value;

template <stage_output ... Outputs>
struct StageOutputs {};

template <typename Container, specifier ... Specifiers>
struct append_stage_outputs {};

template <stage_output ... Outputs, specifier ... Specifiers>
struct append_stage_outputs <StageOutputs <Outputs...>, Specifiers...> {
	using result = StageOutputs <Outputs...>;
};

template <stage_output ... Outputs, specifier S, specifier ... Specifiers>
struct append_stage_outputs <StageOutputs <Outputs...>, S, Specifiers...> {
	using current = StageOutputs <Outputs...>;
	using result = typename append_stage_outputs <current, Specifiers...> ::result;
};

template <stage_output ... Outputs, ShaderStageFlags F, generic T, size_t N, specifier ... Specifiers>
struct append_stage_outputs <StageOutputs <Outputs...>, LayoutOut <F, T, N>, Specifiers...> {
	using current = StageOutputs <Outputs..., StageOut <F, T, N>>;
	using result = typename append_stage_outputs <current, Specifiers...> ::result;
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
// TODO: array based approach
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
	static constexpr auto next = requirement_error_report <Requirements...>();
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
	using raw_requirements = RequirementVector <typename layout_requirement <Specifiers> ::type...>;
	using requirements = typename raw_requirements::filtered;
	using outputs = typename append_stage_outputs <StageOutputs <>, Specifiers...> ::result;
	using satisfied = typename satisfied_mega <requirements, outputs> ::result;

	static constexpr auto errors = typename requirement_error_report_vector <satisfied> ::type();
	static constexpr bool valid = verification_status(satisfied());

	// TODO: extract descriptor set layout
};

// Shader pipeline
struct Pipline {

};

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
