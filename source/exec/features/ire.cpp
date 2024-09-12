#include "ire/core.hpp"

// TODO: immutability for shader inputs types
// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
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
		this->ref = em.emit_construct(qualifier, -1, false);
	}

	// TODO: zero initializing constructor
	template <generic ... Args>
	array(const Args &...args) {
		auto &em = Emitter::active;
		thunder::index_t l = list_from_args(args...);
		thunder::index_t underlying = em.emit_type_information(-1, -1, synthesize_primitive_type <T> ());
		thunder::index_t qualifier = em.emit_qualifier(underlying, N, thunder::arrays);
		this->ref = em.emit_construct(qualifier, l, false);
	}

	element operator[](thunder::index_t index) const {
		MODULE(array);

		JVL_ASSERT(cached(), "arrays must be cached by the time of use");
		if (index < 0 || index >= N)
			JVL_WARNING("index (={}) is out of bounds (size={})", index, N);

		auto &em = Emitter::active;
		primitive_t <int32_t> location(index);
		thunder::index_t l = location.synthesize().id;
		thunder::index_t c = em.emit_array_access(this->ref.id, l);
		return cache_index_t::from(c);
	}

	template <integral_primitive_type U>
	element operator[](const primitive_t <U> &index) const {
		JVL_ASSERT(cached(), "arrays must be cached by the time of use");
		auto &em = Emitter::active;
		thunder::index_t l = index.synthesize().id;
		thunder::index_t c = em.emit_array_access(this->ref.id, l);
		return cache_index_t::from(c);
	}
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
	list[1] = in.x + in.y / in.z;
	return in + list[info.iterations % 3];
};

int main()
{
	// thunder::opt_transform(ftn);
	ftn.dump();

	// fmt::println("{}", ftn.export_to_kernel().compile(profiles::cplusplus_11));
	fmt::println("{}", ftn.export_to_kernel().compile(profiles::glsl_450));
}