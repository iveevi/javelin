#pragma once

#include "../thunder/atom.hpp"
#include "tagged.hpp"
#include "emitter.hpp"
#include "vector.hpp"

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
		thunder::TypeInformation tf;
		tf.item = thunder::mat4;

		thunder::Primitive p;
		p.type = thunder::i32;
		p.idata = 1;

		thunder::List l;
		l.item = em.emit(p);

		thunder::Construct ctor;
		ctor.type = em.emit(tf);
		ctor.args = em.emit(l);

		this->ref = em.emit(ctor);

		return this->ref;
	}
};

template <typename T, size_t N, size_t M>
struct mat : mat_base <T, N, M> {
	using mat_base <T, N, M> ::mat_base;

	friend vec <T, N> operator*(const mat &m, const vec <T, M> &v) {
		return operation_from_args <vec <T, N>> (thunder::multiplication, m, v);
	}
};

} // namespace jvl::ire
