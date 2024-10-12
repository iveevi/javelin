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

// Type-safe wrapper of a tracked buffer
struct CompiledArtifact : thunder::TrackedBuffer {};

// Full function compilation flow
template <typename F>
auto compile_function(const std::string &name, const F &ftn)
{
	// Light-weight semantic checking
	using T = decltype(function_breakdown(ftn));
	using S = signature <typename T::base>;
	using R = typename S::returns;
	using Args = typename S::arguments;

	constexpr auto status = classifier <Args> ::status();

	// TODO: skip code gen if this fails
	verify_classification <status> check;
	
	constexpr auto flags = classifier <Args> ::resolved;

	// TODO: secondary check given the shader stage

	// Begin code generation stage
	CompiledArtifact buffer;
	buffer.name = name;

	auto &em = ire::Emitter::active;
	em.push(buffer);

	if constexpr (has(flags, ShaderStageFlags::eVertex)) {
		Args arguments = typename lift_argument <Args> ::type();
		auto result = std::apply(ftn, arguments);
		exporting(result);
	}
	
	if constexpr (has(flags, ShaderStageFlags::eFragment)) {
		auto arguments = typename lift_argument <Args> ::type();
		auto result = std::apply(ftn, arguments);
		exporting(result);
	}

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
auto fragment_shader(fragment_intrinsics, vec3 pos) -> vec4
{
	return vec4(1, 0, 0, 0);
}

// TODO: vulkan linkage unit

int main()
{
	// TODO: try forwarding tuple to another tuple constructor...

	{
		auto buffer = compile_function("main", vertex_shader);
		thunder::opt_transform(buffer);
		buffer.dump();

		auto s = ire::link(buffer).generate_glsl();
		fmt::println("{}", s);
	}
	
	// {
	// 	auto buffer = compile_function("main", fragment_shader);
	// 	thunder::opt_transform(buffer);
	// 	buffer.dump();

	// 	auto s = ire::link(buffer).generate_glsl();
	// 	fmt::println("{}", s);
	// }
}