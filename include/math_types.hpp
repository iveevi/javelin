#pragma once

#include <cmath>
#include <cstdlib>
#include <unordered_map>

namespace jvl {

#define TEMPLATE_DATA_INDEX                         \
	T &operator[](size_t i) { return data[i]; } \
	const T &operator[](size_t i) const { return data[i]; }

// Math vector types
template <typename T, size_t N>
struct vector {
	T data[N];

	TEMPLATE_DATA_INDEX
};

template <typename T>
struct vector<T, 2> {
	union {
		T data[2] = {0, 0};
		struct {
			T x;
			T y;
		};
	};

	constexpr vector(T x_ = 0, T y_ = 0) : x(x_), y(y_) {}

	TEMPLATE_DATA_INDEX
};

template <typename T>
struct vector<T, 3> {
	union {
		T data[3] = {0, 0, 0};
		struct {
			T x;
			T y;
			T z;
		};
	};

	constexpr vector(T x_ = 0, T y_ = 0, T z_ = 0) : x(x_), y(y_), z(z_) {}

	TEMPLATE_DATA_INDEX
};

template <typename T>
struct vector<T, 4> {
	union {
		T data[4] = {0, 0, 0, 0};
		struct {
			T x;
			T y;
			T z;
			T w;
		};
	};

	constexpr vector(T x_ = 0, T y_ = 0, T z_ = 0, T w_ = 0)
	    : x(x_), y(y_), z(z_), w(w_)
	{
	}

	TEMPLATE_DATA_INDEX
};

template <typename T, size_t N>
inline bool operator==(const vector<T, N> &A, const vector<T, N> &B)
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
	T w = 1;

	vector<T, 3> rotate(const vector<T, 3> &) const;
};

template <typename T, size_t N, size_t M>
struct matrix {
	T data[N][M];

	constexpr matrix(T def = 0)
	{
		for (size_t i = 0; i < N; i++) {
			for (size_t j = 0; j < M; j++)
				data[i][j] = def;
		}
	}

	inline auto operator[](size_t i) { return data[i]; }

	inline auto operator[](size_t i) const { return data[i]; }

	static matrix identity()
	{
		matrix ret;

		size_t m = std::min(N, M);
		for (size_t i = 0; i < m; i++)
			ret.data[i][i] = 1;

		return ret;
	}
};

template <typename T, size_t N, size_t M>
inline bool operator==(const matrix<T, N, M> &A, const matrix<T, N, M> &B)
{
	for (size_t i = 0; i < N; i++) {
		for (size_t j = 0; j < M; j++) {
			if (A.data[i][j] == B.data[i][j])
				return true;
		}
	}

	return false;
}

// Arithmetic operators
// TODO: headers

#define TEMPLATE_VECTOR_OPERATORS(op)                                          \
	template <typename T, size_t N>                                        \
	vector<T, N> operator op(const vector<T, N> &A, const vector<T, N> &B) \
	{                                                                      \
		vector<T, N> C;                                                \
		for (size_t i = 0; i < N; i++)                                 \
			C[i] = A[i] op B[i];                                   \
		return C;                                                      \
	}

#define TEMPLATE_MIXED_VECTOR_OPERATORS(op)                          \
	template <typename T, size_t N>                              \
	vector<T, N> operator op(const vector<T, N> &A, const T & k) \
	{                                                            \
		vector<T, N> C;                                      \
		for (size_t i = 0; i < N; i++)                       \
			C[i] = A[i] op k;                            \
		return C;                                            \
	}                                                            \
	template <typename T, size_t N>                              \
	vector<T, N> operator op(const T & k, const vector<T, N> &A) \
	{                                                            \
		vector<T, N> C;                                      \
		for (size_t i = 0; i < N; i++)                       \
			C[i] = A[i] op k;                            \
		return C;                                            \
	}

TEMPLATE_VECTOR_OPERATORS(+)
TEMPLATE_VECTOR_OPERATORS(-)

TEMPLATE_MIXED_VECTOR_OPERATORS(*)

template <typename T>
matrix<T, 4, 4> operator*(const matrix<T, 4, 4> &A, const matrix<T, 4, 4> &B)
{
	matrix<T, 4, 4> ret;

	ret.data[0][0] =
		A.data[0][0] * B.data[0][0] + A.data[0][1] * B.data[1][0] +
		A.data[0][2] * B.data[2][0] + A.data[0][3] * B.data[3][0];
	ret.data[0][1] =
		A.data[0][0] * B.data[0][1] + A.data[0][1] * B.data[1][1] +
		A.data[0][2] * B.data[2][1] + A.data[0][3] * B.data[3][1];
	ret.data[0][2] =
		A.data[0][0] * B.data[0][2] + A.data[0][1] * B.data[1][2] +
		A.data[0][2] * B.data[2][2] + A.data[0][3] * B.data[3][2];
	ret.data[0][3] =
		A.data[0][0] * B.data[0][3] + A.data[0][1] * B.data[1][3] +
		A.data[0][2] * B.data[2][3] + A.data[0][3] * B.data[3][3];

	ret.data[1][0] =
		A.data[1][0] * B.data[0][0] + A.data[1][1] * B.data[1][0] +
		A.data[1][2] * B.data[2][0] + A.data[1][3] * B.data[3][0];
	ret.data[1][1] =
		A.data[1][0] * B.data[0][1] + A.data[1][1] * B.data[1][1] +
		A.data[1][2] * B.data[2][1] + A.data[1][3] * B.data[3][1];
	ret.data[1][2] =
		A.data[1][0] * B.data[0][2] + A.data[1][1] * B.data[1][2] +
		A.data[1][2] * B.data[2][2] + A.data[1][3] * B.data[3][2];
	ret.data[1][3] =
		A.data[1][0] * B.data[0][3] + A.data[1][1] * B.data[1][3] +
		A.data[1][2] * B.data[2][3] + A.data[1][3] * B.data[3][3];

	ret.data[2][0] =
		A.data[2][0] * B.data[0][0] + A.data[2][1] * B.data[1][0] +
		A.data[2][2] * B.data[2][0] + A.data[2][3] * B.data[3][0];
	ret.data[2][1] =
		A.data[2][0] * B.data[0][1] + A.data[2][1] * B.data[1][1] +
		A.data[2][2] * B.data[2][1] + A.data[2][3] * B.data[3][1];
	ret.data[2][2] =
		A.data[2][0] * B.data[0][2] + A.data[2][1] * B.data[1][2] +
		A.data[2][2] * B.data[2][2] + A.data[2][3] * B.data[3][2];
	ret.data[2][3] =
		A.data[2][0] * B.data[0][3] + A.data[2][1] * B.data[1][3] +
		A.data[2][2] * B.data[2][3] + A.data[2][3] * B.data[3][3];

	ret.data[3][0] =
		A.data[3][0] * B.data[0][0] + A.data[3][1] * B.data[1][0] +
		A.data[3][2] * B.data[2][0] + A.data[3][3] * B.data[3][0];
	ret.data[3][1] =
		A.data[3][0] * B.data[0][1] + A.data[3][1] * B.data[1][1] +
		A.data[3][2] * B.data[2][1] + A.data[3][3] * B.data[3][1];
	ret.data[3][2] =
		A.data[3][0] * B.data[0][2] + A.data[3][1] * B.data[1][2] +
		A.data[3][2] * B.data[2][2] + A.data[3][3] * B.data[3][2];
	ret.data[3][3] =
		A.data[3][0] * B.data[0][3] + A.data[3][1] * B.data[1][3] +
		A.data[3][2] * B.data[2][3] + A.data[3][3] * B.data[3][3];

	return ret;
}

// Other operations
template <typename T, size_t N>
constexpr T dot(const vector<T, N> &A, const vector<T, N> &B)
{
	T lsq = 0;
	for (size_t i = 0; i < N; i++)
		lsq += A[i] * B[i];
	return lsq;
}

template <typename T, size_t N>
constexpr vector<T, N> normalize(const vector<T, N> &v)
{
	T lsq = 0;

#pragma unroll
	for (size_t i = 0; i < N; i++)
		lsq += v[i] * v[i];

	vector<T, N> vn = v;

	lsq = std::sqrt(lsq);

#pragma unroll
	for (size_t i = 0; i < N; i++)
		vn[i] /= lsq;

	return vn;
}

template <typename T>
constexpr vector<T, 3> cross(const vector<T, 3> &A, const vector<T, 3> &B)
{
	vector<T, 3> C;
	C[0] = A[1] * B[2] - A[2] * B[1];
	C[1] = A[2] * B[0] - A[0] * B[2];
	C[2] = A[0] * B[1] - A[1] * B[0];
	return C;
}

template <typename T>
vector<T, 3> quat<T>::rotate(const vector<T, 3> &v) const
{
	const vector<T, 3> u{x, y, z};

	return 2.0f * dot(u, v) * u + (w * w - dot(u, u)) * v +
	       2.0f * w * cross(u, v);
}

// Transform related operations
template <typename T>
inline matrix<T, 3, 3> rotation_to_mat3(const quat<T> &q)
{
	matrix<T, 3, 3> ret;

	ret.data[0][0] = 2 * (q.x * q.x + q.y * q.y) - 1;
	ret.data[0][1] = 2 * (q.y * q.z - q.x * q.w);
	ret.data[0][2] = 2 * (q.y * q.w + q.x * q.z);

	ret.data[1][0] = 2 * (q.y * q.z + q.x * q.w);
	ret.data[1][1] = 2 * (q.x * q.x + q.z * q.z) - 1;
	ret.data[1][2] = 2 * (q.z * q.w - q.x * q.y);

	ret.data[2][0] = 2 * (q.y * q.w - q.x * q.z);
	ret.data[2][1] = 2 * (q.z * q.w + q.x * q.y);
	ret.data[2][2] = 2 * (q.x * q.x + q.w * q.w) - 1;

	return ret;
}

template <typename T>
inline matrix<T, 4, 4> rotation_to_mat4(const quat<T> &q)
{
	matrix<T, 4, 4> ret;

	ret.data[0][0] = 2 * (q.x * q.x + q.y * q.y) - 1;
	ret.data[0][1] = 2 * (q.y * q.z - q.x * q.w);
	ret.data[0][2] = 2 * (q.y * q.w + q.x * q.z);

	ret.data[1][0] = 2 * (q.y * q.z + q.x * q.w);
	ret.data[1][1] = 2 * (q.x * q.x + q.z * q.z) - 1;
	ret.data[1][2] = 2 * (q.z * q.w - q.x * q.y);

	ret.data[2][0] = 2 * (q.y * q.w - q.x * q.z);
	ret.data[2][1] = 2 * (q.z * q.w + q.x * q.y);
	ret.data[2][2] = 2 * (q.x * q.x + q.w * q.w) - 1;

	ret.data[3][3] = 1;

	return ret;
}

template <typename T>
inline matrix<T, 4, 4> translate_to_mat4(const vector<T, 3> &v)
{
	matrix<T, 4, 4> ret = matrix<T, 4, 4>::identity();

	ret[0][3] = v.x;
	ret[1][3] = v.y;
	ret[2][3] = v.z;

	return ret;
}

template <typename T>
inline matrix<T, 4, 4> scale_to_mat4(const vector<T, 3> &s)
{
	matrix<T, 4, 4> ret;

	ret[0][0] = s.x;
	ret[1][1] = s.y;
	ret[2][2] = s.z;
	ret[3][3] = 1;

	return ret;
}

template <typename T>
inline matrix<T, 4, 4> look_at(const vector<T, 3> &eye,
			       const vector<T, 3> &center,
			       const vector<T, 3> &up)
{
	vector<T, 3> f = normalize(center - eye);
	vector<T, 3> u = normalize(up);
	vector<T, 3> s = normalize(cross(f, u));
	u = cross(s, f);

	matrix<T, 4, 4> ret(1);
	ret[0][0] = s.x;
	ret[1][0] = s.y;
	ret[2][0] = s.z;
	ret[0][1] = u.x;
	ret[1][1] = u.y;
	ret[2][1] = u.z;
	ret[0][2] = -f.x;
	ret[1][2] = -f.y;
	ret[2][2] = -f.z;
	ret[3][0] = -dot(s, eye);
	ret[3][1] = -dot(u, eye);
	ret[3][2] = dot(f, eye);
	return ret;
}

// Aliases for practicality
using float2 = vector<float, 2>;
using float3 = vector<float, 3>;

using int3 = vector<int32_t, 3>;
using int4 = vector<int32_t, 4>;

using float3x3 = matrix<float, 3, 3>;
using float4x4 = matrix<float, 4, 4>;

using fquat = quat<float>;

} // namespace jvl

// Hashing for types
template <typename T, size_t N>
struct std::hash<jvl::vector<T, N>> {
	size_t operator()(const jvl::vector<T, N> &v) const
	{
		auto h = hash<float>();

		size_t x = 0;
		for (size_t i = 0; i < N; i++)
			x ^= h(v.data[i]);

		return x;
	}
};
