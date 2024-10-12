#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

namespace jvl::tsg {

using namespace jvl::ire;

struct vertex_intrinsics {};
struct fragment_intrinsics {};
// TODO: put discard here ^
struct compute_intrinsics {};

struct position : vec4 {
	position(const vec4 &other) : vec4(other.ref) {}
};

template <generic T, size_t B, InterpolationKind K = smooth>
struct layout : layout_in <T, K> {
	layout() : layout_in <T, K> (B) {}
};

} // namespace jvl::tsg;

using namespace jvl;
using namespace tsg;

// Injecting into function arguments based on the stage
template <typename T, typename ... Args>
void inject_indexed(size_t I, T &current, Args &... args)
{
	bool layout = false;
	if constexpr (builtin <T>) {
		current = layout_in <T> (I);
		layout = true;
	}

	if constexpr (sizeof...(args))
		return inject_indexed(I + layout, args...);
}

template <typename T, typename ... Args>
void injecting(T &current, Args &... args)
{
	return inject_indexed(0, current, args...);
}

template <typename ... Args>
void injecting(std::tuple <Args...> &args)
{
	using ftn = void (Args &...);
	return std::apply <ftn> (injecting <Args...>, args);
}

// Exporting the returns of a function based on the stage
template <typename T, typename ... Args>
void export_indexed(size_t I, T &current, Args &... args)
{
	bool layout = false;

	if constexpr (std::same_as <T, position>) {
		gl_Position = current;
	} else if constexpr (builtin <T>) {
		layout_out <T> lout(I);
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

enum class UsageFlags : uint16_t {
	eNone		= 0,
	eVertex		= 1 << 0,
	eFragment	= 1 << 1,
	eCompute	= 1 << 2,
	eTask		= 1 << 3,
	eMesh		= 1 << 4,
	eSubroutine	= 1 << 5,
};

enum class ErrorFlags : uint16_t {
	eOk			= 0,
	eDuplicateVertex	= 1 << 0,
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

flag_operators(UsageFlags, uint16_t)
flag_operators(ErrorFlags, uint16_t)

template <typename ... Args>
struct classifier {
	static constexpr UsageFlags flags = UsageFlags::eNone;
	
	// Only so that the default is a subroutine
	static constexpr UsageFlags resolved = UsageFlags::eSubroutine;
	
	static constexpr ErrorFlags status() {
		return ErrorFlags::eOk;
	}
};

template <typename ... Args>
struct classifier <vertex_intrinsics, Args...> {
	using prev = classifier <Args...>;
	static constexpr UsageFlags flags = UsageFlags::eVertex + prev::flags;
	static constexpr UsageFlags resolved = flags;
	
	static constexpr ErrorFlags status() {
		if constexpr (has(prev::flags, UsageFlags::eVertex))
			return prev::status() + ErrorFlags::eDuplicateVertex;

		return prev::status();
	}
};

template <typename ... Args>
struct classifier <std::tuple <Args...>> : classifier <Args...> {};

template <ErrorFlags F>
struct verify_classification {
	static_assert(!has(F, ErrorFlags::eDuplicateVertex));
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

	auto &em = Emitter::active;
	em.push(buffer);

	if constexpr (has(flags, UsageFlags::eVertex)) {
		auto arguments = Args();
		
		injecting(arguments);

		auto result = std::apply(ftn, arguments);

		exporting(result);
	}

	em.pop();

	return buffer;
}

// Testing
std::tuple <position, vec3> vertex_shader(vertex_intrinsics one, vec3 pos)
{
	cond(pos.x > 0.0f);
		pos.y = 1.0f;
	end();

	cond(pos.z == 0.0f);
		discard();
	end();

	return std::make_tuple(vec4(pos, 1), vec3());
}

int main()
{
	auto buffer = compile_function("main", vertex_shader);
	thunder::opt_transform(buffer);
	buffer.dump();

	auto s = link(buffer).generate_glsl();

	fmt::println("{}", s);
}