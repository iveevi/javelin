// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <common/io.hpp>

#include <ire.hpp>

#include <thunder/optimization.hpp>
#include <thunder/linkage_unit.hpp>

using namespace jvl;
using namespace jvl::ire;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: get line numbers for each invocation if possible?
// using source location if available

// TOOD: move to `rexec` module

// Resource implementations
#define DEFINE_RESOURCE_N2(name, concept_suffix, Ta, Tb)			\
	template <Ta A, Tb B>							\
	struct name {};								\
										\
	template <typename T>							\
	struct is_##concept_suffix : std::false_type {};			\
										\
	template <Ta A, Tb B>							\
	struct is_##concept_suffix <name <A, B>> : std::true_type {};		\
										\
	template <typename T>							\
	concept resource_##concept_suffix = is_##concept_suffix <T> ::value;

// TODO: interpolation modes...
DEFINE_RESOURCE_N2(LayoutIn, layout_in,
	generic,	// value type
	size_t		// binding
)

DEFINE_RESOURCE_N2(LayoutOut, layout_out,
	generic,	// value type
	size_t		// binding
)

DEFINE_RESOURCE_N2(PushConstant, push_constant,
	generic,	// value type
	size_t		// offset
)

// TODO: macrofy to enable $layout_in(vec3, 0) or so...

// Concepts
template <typename T>
concept resource = false
	|| resource_layout_in <T>
	|| resource_layout_out <T>
	|| resource_push_constant <T>;

template <resource ... Resources>
struct resource_collection {
	template <resource ... More>
	using append_front = resource_collection <Resources..., More...>;

	// template <resource ... More>
	// using append_back = resource_collection <Resources..., More...>;
};

// TODO: filter specific resources...

// Filtering resources
#define DEFINE_RESOURCE_FILTER(concept_suffix)								\
	template <typename T>										\
	struct filter_##concept_suffix {								\
		using group = void;									\
	};												\
													\
	template <>											\
	struct filter_##concept_suffix <resource_collection <>> {					\
		using group = resource_collection <>;							\
	};												\
													\
	template <resource R, resource ... Rest>							\
	struct filter_##concept_suffix <resource_collection <R, Rest...>> {				\
		using upper = filter_##concept_suffix <resource_collection <Rest...>> ::group;			\
		using fixed = typename upper::template append_front <R>;				\
		using group = std::conditional_t <resource_##concept_suffix <R>, fixed, upper>;		\
	};

DEFINE_RESOURCE_FILTER(layout_in)
DEFINE_RESOURCE_FILTER(layout_out)
DEFINE_RESOURCE_FILTER(push_constant)

// Filling member variables and types based on resources
template <typename T>
struct resource_manifestion_layout_in {
	static_assert(false, "invalid manifestion of layout inputs");
};

// TODO: validation to prevent conflicts
template <>
struct resource_manifestion_layout_in <resource_collection <>> {
	template <size_t B>
	static void layout_in() {
		static_assert(false, "invalid binding location for layout input");
	}
};

template <generic T, size_t B, resource_layout_in ... Inputs>
struct resource_manifestion_layout_in <resource_collection <LayoutIn <T, B>, Inputs...>> {
	using next = resource_manifestion_layout_in <resource_collection <Inputs...>>;

	template <size_t I>
	static auto layout_in() {
		if constexpr (I == B)
			return ire::layout_in <T> (B);
		else
			return next::template layout_in <I> ();
	}
};

template <resource ... Resources>
using manifest_layout_in = resource_manifestion_layout_in <typename filter_layout_in <resource_collection <Resources...>> ::group>;

template <typename T>
struct resource_manifestion_push_constant {
	static_assert(false, "invalid manifestion of push constants");
};

template <generic T, size_t Offset>
struct resource_manifestion_push_constant <resource_collection <PushConstant <T, Offset>>> {
	static auto push_constant() {
		return ire::push_constant <T> (Offset);
	}
};

template <resource_push_constant ... Resources>
struct resource_manifestion_push_constant <resource_collection <Resources...>> {
	static_assert(false, "only one push constant is allowed per context");
};

// TODO: macro define_manifestation
template <resource ... Resources>
using manifest_push_constant = resource_manifestion_push_constant <typename filter_push_constant <resource_collection <Resources...>> ::group>;

template <resource ... Resources>
struct resource_manifestion :
	manifest_layout_in <Resources...>,
	manifest_push_constant <Resources...>
	{};

// Sugared accessors
#define $lin(I)		layout_in <I> ()
#define $lout(I)	layout_out <I> ()
#define $constants	push_constant()

// Resource compatiblity
template <typename Super, typename Sub>
struct is_resource_subset : std::false_type {};

template <resource ... Supers, resource ... Subs>
struct is_resource_subset <
	resource_collection <Supers...>,
	resource_collection <Subs...>
> {
	template <resource S>
	static constexpr bool contained = (std::same_as <S, Supers> || ...);

	static constexpr bool value = (contained <Subs> && ...);
};

template <typename T, typename U>
concept resource_subset = is_resource_subset <T, U> ::value;

// Resource and execution context
enum class ExecutionFlag : uint8_t {
	eInvalid,
	eSubroutine,
	eVertex,
	eFragment,
};

template <ExecutionFlag Flag, resource ... Resources>
struct ResourceExecutionContext : resource_manifestion <Resources...> {
	static constexpr ExecutionFlag flag = Flag;

	using collection = resource_collection <Resources...>;
};

template <ExecutionFlag Flag, resource ... Resources>
bool rexec_pass(const ResourceExecutionContext <Flag, Resources...> &);

template <typename T>
concept rexec = requires(const T &x) {
	{ rexec_pass(x) } -> std::same_as <bool>;
};

template <resource ... Resources>
struct Callable : ResourceExecutionContext <ExecutionFlag::eSubroutine, Resources...> {
	// Nothing extra here...
	// can only import other callables
	
	template <rexec T>
	static T _use() {
		using collection = resource_collection <Resources...>;

		// Ensure execution compatibility
		static_assert(T::flag == ExecutionFlag::eSubroutine,
			"only subroutines are allowed as sub-RECs");

		// Ensure resource compatibility
		static_assert(resource_subset <collection, typename T::collection>,
			"imported scope uses resource not specified in the current REC");

		return T();
	}
};

// Type safe options: shaders as functors...
template <resource ... Resources>
struct Vertex : ResourceExecutionContext <ExecutionFlag::eVertex, Resources...> {
	static gl_Position_t gl_Position;
	// TODO: import the rest of the instrinsics here...

	// TODO: specify rasterization options here as well?

	template <rexec T>
	static T _use() {
		using collection = resource_collection <Resources...>;

		// Ensure execution compatibility
		static_assert(T::flag == ExecutionFlag::eSubroutine,
			"only subroutines are allowed as sub-RECs");

		// Ensure resource compatibility
		static_assert(resource_subset <collection, typename T::collection>,
			"imported scope uses resource not specified in the current REC");

		return T();
	}
};

template <resource ... Resources>
gl_Position_t Vertex <Resources...> ::gl_Position;

struct Fragment {
};

#define $rexec_callable(name, ...)	struct name : Callable <__VA_ARGS__>
#define $rexec_vertex(name, ...)	struct name : Vertex <__VA_ARGS__>

#define $layout_in(T, B, ...)	LayoutIn <T, B>
#define $layout_out(T, B)	LayoutOut <T, B>
#define $push_constant(T, O)	PushConstant <T, O>

#define $use(id) _use <id> ()

// $rec_... variants are simply static inline qualified
#define $rexec_subroutine(...)	static inline $subroutine(__VA_ARGS__)
#define $rexec_entrypoint(...)	static inline $entrypoint(__VA_ARGS__)

#define $declare_subroutine(R, name, ...)					\
	::jvl::ire::manifest_skeleton <R, void (*)(__VA_ARGS__)> ::proc name

#define $declare_rexec_subroutine(...)	static $declare_subroutine(__VA_ARGS__)

#define $implement_rexec_subroutine(rexec, R, name, ...)				\
	::jvl::ire::manifest_skeleton <R, void (*)(__VA_ARGS__)> ::proc rexec::name	\
		= ::jvl::ire::ProcedureBuilder <R> (#name)				\
		<< [_returner = _return_igniter <R> ()](__VA_ARGS__) -> void

// In a separate module, author a shader module
// which defines an environment of available
// resources and the corresponding methods under that scope...

// Example:
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};

$rexec_callable(Projection, $push_constant(MVP, 0))
{
	// Forward declaring to define an interface...
	$declare_rexec_subroutine(vec4, project, vec3);
};

// ...and write the implementation internally...
$implement_rexec_subroutine(Projection, vec4, project, vec3 p)
{
	$return $constants.project(p);
};

$rexec_vertex(VShader, $layout_in(vec3, 0), $push_constant(MVP, 0))
{
	$rexec_entrypoint(main) {
		// ...such that the REXEC can still be used appropriately
		gl_Position = $use(Projection).project($lin(0));
	};
};

// TODO: automatic pipeline generation...

// TODO: linking raw shader (string) code as well...
// use as a test for shader toy examples

int main()
{
	auto glsl = link(VShader::main).generate_glsl();

	io::display_lines("GLSL", glsl);

	VShader::main.display_assembly();
}
