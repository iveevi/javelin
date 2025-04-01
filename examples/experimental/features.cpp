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

// TODO: manifest <resource_group> adds all the necessary members...
// then it becomes
// 
// vec3 p = layout_in <0> (); // or $lin(0)
// 
// layout_out <0> () = vec4(color, 1); // or $lout(0)
//
// and likewise for buffers, textures, etc.resource Resources

// Resource and execution context
enum class ExecutionFlag : uint8_t {
	eSubroutine,
	eVertex,
	eFragment,
};

template <ExecutionFlag, resource ... Resources>
struct ResourceExecutionContext {};

// Type safe options: shaders as functors...
template <resource ... Resources>
struct Vertex : resource_manifestion <Resources...> {
	static gl_Position_t gl_Position;

	// TODO: specify rasterization options here as well?

	// // Override return capabilities
	// struct _return_igniter {
	// 	void operator<<(_void) const {
	// 		fmt::println("Vertex return!");
	// 		return _return();
	// 	}
	// };

	const auto &_import() {

	}

	// TODO: import other scopes...
	// i.e. $import(object) ftn(args...)
	//    -> _import(object).ftn(args...);
};

template <resource ... Resources>
gl_Position_t Vertex <Resources...> ::gl_Position;

struct Fragment {
};

template <generic_or_void R, resource ... Resources>
struct Subroutine : resource_manifestion <Resources...> {
	// // Override return capabilities
	// struct _return_igniter {
	// 	void operator<<(R x) const {
	// 		fmt::println("Subroutine return!");
	// 		return _return(x);
	// 	}
	// };
};

// Examples...

// In a separate module, author a shader module
// which defines an environment of available
// resources and the corresponding methods under that scope...

// struct Method : Subroutine <vec3> {
// 	// void operator()() {
// 	// 	$return vec3(1);
// 	// };
// } method;

// REC -- resource and execution (i.e. stage) context

// TODO: macrofy to $rec(Vertex, resources...)
// #define $rec_subroutine(R, ...)		struct : Subroutine <R, __VA_ARGS__>
// #define $rec_vertex(...)		struct : Vertex <__VA_ARGS__>

#define $layout_in(T, B, ...)	LayoutIn <T, B>
#define $layout_out(T, B)	LayoutOut <T, B>
#define $push_constant(T, O)	PushConstant <T, O>

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

// TODO: template offset parameter?
// $rec_subroutine
// (
// 	// Return type for all subsequent methods
// 	vec3,
// 	// Resources used in this context
// 	$push_constant(MVP, 0)
// )


struct _projection : Subroutine <vec4, $push_constant(MVP, 0)>
{
	$subroutine(vec4, project, vec3 p) {
		$return $constants.project(p);
	};
} projection;

// $rec_vertex
// (
// 	// Resources used in this context
// 	$layout_in(vec3, 0),
// 	$push_constant(MVP, 0)
// )

struct _vshader : Vertex <
	$layout_in(vec3, 0),
	$push_constant(MVP, 0)
>
{
	// Subroutines can be specified inside
	// which will respect the resource constraints...
	$subroutine(vec4, project, vec3 p) {
		$return $constants.project(p);
	};

	$entrypoint(main) {
		gl_Position = $constants.project($lin(0));
		// gl_Position = projection.project($lin(0));
	};
} vshader;

// TODO: above becomes
// $method(name, resources) {
//      void operator()(args...) override {
//      }
// }
// 
// with a call operator which is partial specializable...
// TODO: some way to check that resources are available when
// calling subsequent methods...
// 
// ultimately this become procedure level abstraction and modularity...

// TODO: and then partial specializable strong procedures ones...

// TODO: linking raw shader (string) code as well...
// use as a test for shader toy examples

template <integral_arithmetic T>
$subroutine(T, ftn, T x)
{
	$return (x << 2 | x & 0b10);
};

template <integral_arithmetic T>
$partial_subroutine(T, partial_ftn, int32_t shamt, T x)
{
	$return (x << shamt | x & 0b10);
};

// TODO: type checking for return statements...

using collection = resource_collection <
	LayoutIn <vec3, 0>,
	LayoutOut <vec3, 0>,
	LayoutIn <vec3, 1>
>;

using layout_inputs = filter_layout_in <collection> ::group;
using layout_outputs = filter_layout_out <collection> ::group;

$subroutine(f32, name, f32 x)
{
	$return x * x;
};

int main()
{
	// auto glsl = link(vshader.main).generate_glsl();
	// auto glsl = link(name).generate_glsl();

	// io::display_lines("GLSL", glsl);

	// vshader.main.display_assembly();

	ftn <i32> .display_assembly();
	ftn <u32> .display_assembly();
	
	partial_ftn <i32> (1).display_assembly();
	partial_ftn <u32> (3).display_assembly();
}
