#pragma once

#include "tagged.hpp"

namespace jvl::ire {

// Matrix types
template <typename T, size_t N, size_t M>
struct mat_base : tagged {
	T data[N][M];

	// TODO: operator[]
	cache_index_t synthesize() const {
		if (this->cached())
			return this->ref;

		auto &em = Emitter::active;

		// TODO: manual assignment, except for mat2x2
		// TODO: instantiate all primitive types at header
		op::TypeField tf;
		tf.item = op::mat4;

		op::Primitive p;
		p.type = op::i32;
		p.idata[0] = 1;

		op::List l;
		l.item = em.emit(p);

		op::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = em.emit(l);

		return (this->ref = em.emit(ctor));
	}
};

template <typename T, size_t N, size_t M>
struct mat : mat_base <T, N, M> {
	using mat_base <T, N, M> ::mat_base;
};

} // namespace jvl::ire
