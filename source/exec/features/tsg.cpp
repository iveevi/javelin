#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

namespace jvl::tsg {

// Import declarations from the IRE
using ire::generic;

using ire::vec;

using ire::i32;

using ire::f32;
using ire::vec2;
using ire::vec3;
using ire::vec4;

using ire::end;
using ire::cond;
using ire::loop;

// Shader stage intrinsics, used to indicate the
// shader stage of a shader program metafunction
struct vertex_intrinsics {};
struct fragment_intrinsics {};
// TODO: put discard here ^
struct compute_intrinsics {};

struct position : vec4 {
	position(const vec4 &other) : vec4(other.ref) {}
};

// interpolation mode wrappers
template <generic T>
struct flat {};

template <generic T>
struct smooth {};

template <generic T>
struct noperspective;

} // namespace jvl::tsg;

using namespace jvl;
using namespace tsg;

// Recursive initialization of lifted arguments
template <typename T, typename ... Args>
struct lift_initializer {
	static void run(size_t binding, T &current, Args &...args) {
		if constexpr (sizeof...(args))
			return lift_initializer <Args...> ::run(binding, args...);
	}
};

template <typename T, typename ... Args>
struct lift_initializer <ire::layout_in <T>, Args...> {
	static void run(size_t binding, ire::layout_in <T> &current, Args &... args) {	
		current = ire::layout_in <T> (binding);
		if constexpr (sizeof...(args))
			return lift_initializer <Args...> ::run(binding + 1, args...);
	}
};

template <typename ... Args>
void lift_initialize_starter(Args &... args)
{
	return lift_initializer <Args...> ::run(0, args...);
}

template <typename ... Args>
void lift_initialize_arguments(std::tuple <Args...> &args)
{
	return std::apply(lift_initialize_starter <Args...>, args);
}

// Lifting arguments
// TODO: parameterize by stage using a template class?
template <typename T>
struct lift_argument {
	using type = T;
};

template <generic T>
struct lift_argument <T> {
	using type = ire::layout_in <T>;
};

template <typename ... Args>
struct lift_argument <std::tuple <Args...>> {
	using type = std::tuple <typename lift_argument <Args> ::type...>;
};

// Lifting shader outputs
// TODO: parameterize by stage using a template class?
template <typename T>
struct lift_result {
	using type = T;
};

template <>
struct lift_result <position> {
	using type = position;
};

template <generic T>
struct lift_result <T> {
	using type = ire::layout_out <T>;
};

template <typename ... Args>
struct lift_result <std::tuple <Args...>> {
	using type = std::tuple <typename lift_result <Args> ::type...>;
};

// Exporting the returns of a function based on the stage
template <typename T, typename ... Args>
void export_indexed(size_t I, T &current, Args &... args)
{
	bool layout = false;

	if constexpr (std::same_as <T, position>) {
		ire::gl_Position = current;
	} else if constexpr (ire::builtin <T>) {
		ire::layout_out <T> lout(I);
		lout = current;
		layout = true;
	}

	if constexpr (sizeof...(args))
		return export_indexed(I + layout, args...);
}

template <typename T, typename ... Args>
void exporting(T &current, Args &... args)
{
	return export_indexed(0, current, args...);
}

template <typename ... Args>
void exporting(std::tuple <Args...> &args)
{
	using ftn = void (Args &...);
	return std::apply <ftn> (exporting <Args...>, args);
}

// Resolving the function meta information
#define hacked(T) *reinterpret_cast <T *> ((void *) nullptr)

template <typename F>
auto std_function(const F &ftn)
{
	return std::function(ftn);
}

template <typename F>
struct function_type_breakdown {
	using base = decltype(std_function(hacked(F)));
};

template <typename F>
auto function_breakdown(const F &ftn)
{
	return function_type_breakdown <F> ();
}

template <typename F>
struct signature {};

template <typename R, typename ... Args>
struct signature <std::function <R (Args...)>> {
	using returns = R;
	using arguments = std::tuple <std::decay_t <Args>...>;
};

enum class ShaderStageFlags : uint16_t {
	eNone		= 0,
	eVertex		= 1 << 0,
	eFragment	= 1 << 1,
	eVxF		= eVertex | eFragment,
	eCompute	= 1 << 2,
	eTask		= 1 << 3,
	eMesh		= 1 << 4,
	eSubroutine	= 1 << 5,
};

enum class ClassificationErrorFlags : uint16_t {
	eOk			= 0,
	eDuplicateVertex	= 1 << 0,
	eDuplicateFragment	= 1 << 1,
	eMultipleStages		= 1 << 2,
};

#define flag_operators(T, base)						\
	constexpr T operator+(T A, T B)					\
	{								\
		return T(base(A) | base(B));				\
	}								\
	constexpr bool has(T A, T B)					\
	{								\
		return ((base(A) & base(B)) == base(B));		\
	}

flag_operators(ShaderStageFlags, uint16_t)
flag_operators(ClassificationErrorFlags, uint16_t)

template <typename ... Args>
struct classifier {
	static constexpr ShaderStageFlags flags = ShaderStageFlags::eNone;
	
	// Only so that the default is a subroutine
	static constexpr ShaderStageFlags resolved = ShaderStageFlags::eSubroutine;
	
	static constexpr ClassificationErrorFlags status() {
		return ClassificationErrorFlags::eOk;
	}
};

template <typename ... Args>
struct classifier <vertex_intrinsics, Args...> {
	using prev = classifier <Args...>;
	static constexpr ShaderStageFlags flags = ShaderStageFlags::eVertex;
	static constexpr ShaderStageFlags resolved = flags;
	
	static constexpr ClassificationErrorFlags status() {
		if constexpr (has(prev::flags, ShaderStageFlags::eVertex))
			return prev::status() + ClassificationErrorFlags::eDuplicateVertex;

		if constexpr (prev::flags != ShaderStageFlags::eNone)
			return prev::status() + ClassificationErrorFlags::eMultipleStages;

		return prev::status();
	}
};

template <typename ... Args>
struct classifier <fragment_intrinsics, Args...> {
	using prev = classifier <Args...>;
	static constexpr ShaderStageFlags flags = ShaderStageFlags::eFragment;
	static constexpr ShaderStageFlags resolved = flags;
	
	static constexpr ClassificationErrorFlags status() {
		if constexpr (has(prev::flags, ShaderStageFlags::eFragment))
			return prev::status() + ClassificationErrorFlags::eDuplicateFragment;
		
		if constexpr (prev::flags != ShaderStageFlags::eNone)
			return prev::status() + ClassificationErrorFlags::eMultipleStages;

		return prev::status();
	}
};

template <typename ... Args>
struct classifier <std::tuple <Args...>> : classifier <Args...> {};

template <ClassificationErrorFlags F>
struct verify_classification {
	static_assert(!has(F, ClassificationErrorFlags::eDuplicateVertex),
		"Vertexshader program must use a single stage intrinsic parameter");

	static_assert(!has(F, ClassificationErrorFlags::eDuplicateFragment),
		"Fragment shader program must use a single stage intrinsic parameter");

	static_assert(!has(F, ClassificationErrorFlags::eMultipleStages),
		"Shader program cannot use multiple stage intrinsics");
};

// Specifiers for a shader program
template <ShaderStageFlags F, generic T, size_t N>
struct LayoutIn {};

template <ShaderStageFlags F, generic T, size_t N>
struct LayoutOut {};

// Concepts for layout inputs
template <typename T>
struct is_layout_in_type : std::false_type {};

template <ShaderStageFlags F, generic T, size_t N>
struct is_layout_in_type <LayoutIn <F, T, N>> : std::true_type {};

template <typename T>
concept layout_input = is_layout_in_type <T> ::value;

// Concepts for layout outputs
template <typename T>
struct is_layout_out_type : std::false_type {};

template <ShaderStageFlags F, generic T, size_t N>
struct is_layout_out_type <LayoutOut <F, T, N>> : std::true_type {};

template <typename T>
concept layout_output = is_layout_out_type <T> ::value;

// Set of all specifier types
template <typename T>
concept specifier = is_layout_in_type <T> ::value || is_layout_out_type <T> ::value;

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
	lift_initialize_arguments(arguments);
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

// Shader linkage unit
template <ShaderStageFlags flags = ShaderStageFlags::eNone, typename ... Args>
struct ShaderProgram {};

// From zero
template <>
struct ShaderProgram <ShaderStageFlags::eNone> {
	template <ShaderStageFlags added, specifier ... Specifiers>
	friend auto operator<<(const ShaderProgram &program, const CompiledArtifact <added, Specifiers...> &compiled) {
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
	friend auto operator<<(const ShaderProgram &program, const CompiledArtifact <ShaderStageFlags::eFragment, AddedSpecifiers...> &compiled) {
		ShaderProgram <ShaderStageFlags::eVxF,
			       Specifiers...,
			       AddedSpecifiers...> result {
			program.ir_vertex,
			static_cast <thunder::TrackedBuffer> (compiled)
		};

		return result;
	}
};

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
	  ShaderStageFlags	RequiredFrom,
	  generic		Type,
	  size_t		Binding>
struct RequirementOut {};

struct SatisfiedRequirement {};
struct EmptyRequirement {};

template <typename T>
struct is_requirement_type : std::false_type {};

template <ShaderStageFlags S, ShaderStageFlags F, generic T, size_t N>
struct is_requirement_type <RequirementOut <S, F, T, N>> : std::true_type {};

template <>
struct is_requirement_type <EmptyRequirement> : std::true_type {};

template <>
struct is_requirement_type <SatisfiedRequirement> : std::true_type {};

template <typename T>
concept requirement = is_requirement_type <T> ::value;

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
	using type = RequirementOut <ShaderStageFlags::eFragment,
				     ShaderStageFlags::eVertex,
				     T, N>;
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

template <ShaderStageFlags S, ShaderStageFlags F, generic T, size_t N, stage_output ... Outputs>
struct satisfied <RequirementOut <S, F, T, N>, StageOut <F, T, N>, Outputs...> {
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

template <ShaderStageFlags S, ShaderStageFlags F, generic T, size_t N>
struct dedicated_reporter {
	static_assert(false, "shader dependency is unsatisfied");
};

template <ShaderStageFlags S, ShaderStageFlags F, generic T, size_t N, requirement ... Requirements>
struct requirement_error_report <RequirementOut <S, F, T, N>, Requirements...> {
	static constexpr auto next = requirement_error_report <Requirements...>();
	static constexpr auto reporter = dedicated_reporter <S, F, T, N> ();
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

template <ShaderStageFlags S, ShaderStageFlags F, generic T, size_t N>
struct requirement_status <RequirementOut <S, F, T, N>> {
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

// Layout verification
template <specifier ... Specifiers>
struct ShaderProgram <ShaderStageFlags::eVxF, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;
	thunder::TrackedBuffer ir_fragment;
	
	using raw_requirements = RequirementVector <typename layout_requirement <Specifiers> ::type...>;
	using requirements = typename raw_requirements::filtered;
	using outputs = typename append_stage_outputs <StageOutputs <>, Specifiers...> ::result;
	using satisfied = typename satisfied_mega <requirements, outputs> ::result;

	static constexpr auto errors = typename requirement_error_report_vector <satisfied> ::type();
	static constexpr bool valid = verification_status(satisfied());

	// TODO: extract descriptor set latyout
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

	next.ir_vertex.dump();
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