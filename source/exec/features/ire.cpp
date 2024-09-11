#include <fmt/format.h>
#include <type_traits>

#include "ire/aggregate.hpp"
#include "ire/aliases.hpp"
#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/solidify.hpp"
#include "ire/tagged.hpp"
#include "ire/uniform_layout.hpp"
#include "profiles/targets.hpp"
#include "thunder/linkage.hpp"
#include "thunder/opt.hpp"
#include "math_types.hpp"
#include "logging.hpp"
#include "constants.hpp"

// TODO: immutability for shader inputs types
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: synthesizable with name hints
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: parameter qualifiers (e.g. out/inout) as wrapped types
// TODO: array types
// TODO: binary operations
// TODO: external constant specialization

using namespace jvl;
using namespace jvl::ire;

// TODO: unsized arrays as well

template <generic T, thunder::index_t N>
struct array {};

// TODO: array base with constructors, and then array with operator[]
template <primitive_type T, thunder::index_t N>
struct array <T, N> : public tagged {
	using element = primitive_t <T>;

	array() {
		auto &em = Emitter::active;
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, -1, true);
	}

	template <generic U, generic ... Args>
	void __initialize(thunder::index_t index, const U &arg, const Args &... args) {
		JVL_ASSERT(cached(), "arrays must be cached before initialization");

		auto &em = Emitter::active;

		element e(arg);
		thunder::index_t indexed = em.emit_load(this->ref.id, index);
		thunder::index_t stored = em.emit_store(indexed, e.synthesize().id, true);

		if constexpr (sizeof...(Args))
			__initialize(index + 1, args...);
	}

	template <generic ... Args>
	array(const Args &...args) : array() {
		__initialize <Args...> (0, args...);
	}

	// TODO: index with primitive int
	element operator[](thunder::index_t index) const {
		JVL_ASSERT(cached(), "arrays must be cached by the time of use");
		
		auto &em = Emitter::active;
		cache_index_t ci;
		ci = em.emit_load(this->ref.id, index);
		return ci;
	}

	// TODO: zero initializing constructor
};

struct shuffle_info {
	i32 iterations;
	i32 stride;

	auto layout() const {
		return uniform_layout("shuffle_info",
			named_field(iterations),
			named_field(stride));
	}
};

auto ftn = callable_info("shuffle") >> [](ivec3 in, shuffle_info info)
{
	// TODO: color wheel generator
	array <int, 3> list { 1, 2, 3 };
	return in + list[1];
};

int main()
{
	ftn.dump();
	thunder::opt_transform(ftn);
	ftn.dump();

	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));
	fmt::println("{}", ftn.export_to_kernel().compile(profiles::glsl_450));
}