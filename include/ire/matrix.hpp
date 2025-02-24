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

	using tagged::tagged;

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

	static constexpr thunder::PrimitiveType primitive() {
		if constexpr (std::same_as <T, float>) {
			switch (N) {
			case 2:
				return M == 2 ? thunder::mat2 : thunder::bad;
			case 3:
				return M == 3 ? thunder::mat3 : thunder::bad;
			case 4:
				return M == 4 ? thunder::mat4 : thunder::bad;
			default:
				break;
			}
		}

		return thunder::bad;
	}
};

template <typename T, size_t N, size_t M>
struct mat : mat_base <T, N, M> {
	using mat_base <T, N, M> ::mat_base;
	
	using native_type = T;
	using arithmetic_type = mat <T, N, M>;

	using result_type = vec <T, M>;
	using input_type = vec <T, N>;
};

// Matrix concept
template <typename T>
struct is_matrix_base : std::false_type {};

template <native T, size_t N, size_t M>
struct is_matrix_base <mat <T, N, M>> : std::true_type {};

template <typename T>
concept matrix_arithmetic = is_matrix_base <typename T::arithmetic_type> ::value;

// Operations with matrices
template <matrix_arithmetic T>
auto operator*(const T &m, const typename T::arithmetic_type::input_type &v)
{
	using result = typename T::arithmetic_type::result_type;
	return operation_from_args <result> (thunder::multiplication, m, v);
}

// Override type generation
template <native T, size_t N, size_t M>
struct type_info_generator <mat <T, N, M>> {
	type_info_generator(const mat <T, N, M> &) {}

	intermediate_type synthesize() {
		return primitive_type(mat <T, N, M> ::primitive());
	}
};

} // namespace jvl::ire
