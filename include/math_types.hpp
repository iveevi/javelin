#pragma once

#include <unordered_map>
#include <cstdlib>

namespace jvl {

// Forward declarations
template <typename T, size_t N, size_t M>
struct matrix;

// Math vector types
template <typename T, size_t N>
struct vector {
	T data[N];
};

template <typename T>
struct vector <T, 2> {
	union {
		T data[2] = { 0, 0 };
		struct {
			T x;
			T y;
		};
	};

	vector(T x_ = 0, T y_ = 0)
			: x(x_), y(y_) {}
};

template <typename T>
struct vector <T, 3> {
	union {
		T data[3] = { 0, 0, 0 };
		struct {
			T x;
			T y;
			T z;
		};
	};

	vector(T x_ = 0, T y_ = 0, T z_ = 0)
			: x(x_), y(y_), z(z_) {}
};

template <typename T>
struct vector <T, 4> {
	union {
		T data[4] = { 0, 0, 0, 0 };
		struct {
			T x;
			T y;
			T z;
			T w;
		};
	};

	vector(T x_ = 0, T y_ = 0, T z_ = 0, T w_ = 0)
			: x(x_), y(y_), z(z_), w(w_) {}
};

template <typename T, size_t N>
inline bool operator==(const vector <T, N> &A, const vector <T, N> &B)
{
	for (size_t i = 0; i < N; i++) {
		if (A.data[i] == B.data[i])
			return true;
	}

	return false;
}

template <typename T>
struct quat {
	T x = 0;
	T y = 0;
	T z = 0;
	T w = 0;

	matrix <T, 3, 3> to_mat3() const;
};

template <typename T, size_t N, size_t M>
struct matrix {
	T data[N][M];

	constexpr matrix() {
		for (size_t i = 0; i < N; i++) {
			for (size_t j = 0; j < M; j++)
				data[i][j] = 0;
		}
	}

	// TODO: convertable type? (explicit version)
	template <size_t L, size_t K>
	requires (L <= N) && (K <= M)
	constexpr matrix(const matrix <T, L, K> &other) : matrix() {
		for (size_t i = 0; i < L; i++) {
			for (size_t j = 0; j < K; j++)
				data[i][j] = other[i][j];
		}
	}

	inline auto operator[](size_t i) {
		return data[i];
	}

	inline auto operator[](size_t i) const {
		return data[i];
	}
};

// Conversion operations
template <typename T>
inline matrix <T, 3, 3> quat <T> ::to_mat3() const
{
	matrix <T, 3, 3> ret;

	ret.data[0][0] = 2 * (x * x + y * y) - 1;
	ret.data[0][1] = 2 * (y * z - x * w);
	ret.data[0][2] = 2 * (y * w + x * z);

	ret.data[1][0] = 2 * (y * z + x * w);
	ret.data[1][1] = 2 * (x * x + z * z) - 1;
	ret.data[1][2] = 2 * (z * w - x * y);

	ret.data[2][0] = 2 * (y * w - x * z);
	ret.data[2][1] = 2 * (z * w + x * y);
	ret.data[2][2] = 2 * (x * x + w * w) - 1;

	return ret;
}

// Aliases for practicality
using float2 = vector <float, 2>;
using float3 = vector <float, 3>;

using int3 = vector <int32_t, 3>;
using int4 = vector <int32_t, 4>;

using float3x3 = matrix <float, 3, 3>;
using float4x4 = matrix <float, 4, 4>;

using fquat = quat <float>;

}

// Hashing for types
template <typename T, size_t N>
struct std::hash <jvl::vector <T, N>> {
	size_t operator()(const jvl::vector <T, N> &v) const {
		auto h = hash <float> ();

		size_t x = 0;
		for (size_t i = 0; i < N; i++)
			x ^= h(v.data[i]);

		return x;
	}
};

