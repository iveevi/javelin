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
auto vertex_shader(vertex_intrinsics, vec3 pos) -> std::tuple <position, vec3>
{
	cond(pos.x > 0.0f);
		pos.y = 1.0f;
	end();

	return std::make_tuple(vec4(pos, 1), vec3());
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

template <specifier ... Specifiers>
struct ShaderProgram <ShaderStageFlags::eVxF, Specifiers...> {
	thunder::TrackedBuffer ir_vertex;
	thunder::TrackedBuffer ir_fragment;
};

// Shader pipeline
struct Pipline {

};
	
using T = decltype(function_breakdown(vertex_shader));
using S = signature <typename T::base>;
using R = typename S::returns;
using Args = typename S::arguments;
constexpr auto flags = classifier <Args> ::resolved;

using LiftedR = typename lift_result <R> ::type;
using LiftedArgs = typename lift_argument <Args> ::type;
using Dependencies = typename layout_input_collector_args <flags, LiftedArgs> ::type;
using Result = typename layout_output_collector_args <flags, LiftedR> ::type;

int main()
{
	auto vs = compile_function("main", vertex_shader);
	auto fs = compile_function("main", fragment_shader);

	auto unit = ShaderProgram();
	auto next = unit << vs << fs;

	// auto fs = compile_function("main", fragment_shader);

	// auto unit = ShaderProgram() << vs << fs;
	
	// fmt::println("{}", ire::link(fs).generate_glsl());

	// {
	// 	auto buffer = compile_function("main", vertex_shader);
	// 	thunder::opt_transform(buffer);
	// 	buffer.dump();

	// 	auto s = ire::link(buffer).generate_glsl();
	// 	fmt::println("{}", s);
	// }
	
	// {
	// 	auto buffer = compile_function("main", fragment_shader);
	// 	thunder::opt_transform(buffer);
	// 	buffer.dump();

	// 	auto s = ire::link(buffer).generate_glsl();
	// 	fmt::println("{}", s);
	// }
}