#pragma once

#include "emitter.hpp"
#include "tagged.hpp"

namespace jvl::ire {

// Way to upcast C++ primitives into a workable type
inline int translate_primitive(bool b)
{
	auto &em = Emitter::active;

	op::Primitive p;
	p.type = op::boolean;
	p.idata[0] = b;

	return em.emit(p);
}

inline int translate_primitive(int i)
{
	auto &em = Emitter::active;

	op::Primitive p;
	p.type = op::i32;
	p.idata[0] = i;

	return em.emit(p);
}

inline int translate_primitive(float f)
{
	auto &em = Emitter::active;

	op::Primitive p;
	p.type = op::f32;
	p.fdata[0] = f;

	return em.emit(p);
}

template <typename T>
concept gltype_complatible = requires(const T &t) {
	{
		translate_primitive(t)
	} -> std::same_as <int>;
};

template <gltype_complatible T>
struct gltype : tagged {
	using tagged::tagged;

	T value;

	gltype(const T &v = T()) : tagged(), value(v) {
		synthesize();
	}

	gltype &operator=(const T &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		em.mark_used(ref.id, true);

		op::Store store;
		store.dst = ref.id;
		store.src = translate_primitive(v);
		em.emit_main(store);

		return *this;
	}

	gltype &operator=(const gltype &v) {
		// At this point we are required to have storage for this
		auto &em = Emitter::active;
		em.mark_used(ref.id, true);

		op::Store store;
		store.dst = ref.id;
		store.src = v.synthesize().id;
		em.emit_main(store);

		return *this;
	}

	cache_index_t synthesize() const {
		if (cached())
			return ref;

		return (ref = translate_primitive(value));
	}
};

} // namespace jvl::ire