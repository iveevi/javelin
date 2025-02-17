#pragma once

/////////////////////////
// Method constructors //
/////////////////////////

#define SWIZZLE2(name, c1, c2)									\
	vec <T, 2> name() const {								\
		auto &em = Emitter::active;							\
		auto args = list_from_args(c1, c2);						\
		auto ctor = em.emit_construct(vec <T, 2> ::type(), args, thunder::normal);	\
		return cache_index_t::from(ctor);						\
	}

#define SWIZZLE3(name, c1, c2, c3)								\
	vec <T, 3> name() const {								\
		auto &em = Emitter::active;							\
		auto args = list_from_args(c1, c2, c3);						\
		auto ctor = em.emit_construct(vec <T, 3> ::type(), args, thunder::normal);	\
		return cache_index_t::from(ctor);						\
	}

#define SWIZZLE4(name, c1, c2, c3, c4)								\
	vec <T, 4> name() const {								\
		auto &em = Emitter::active;							\
		auto args = list_from_args(c1, c2, c3, c4);					\
		auto ctor = em.emit_construct(vec <T, 4> ::type(), args, thunder::normal);	\
		return cache_index_t::from(ctor);						\
	}

////////////////////////////
// Two-dimensional vector //
////////////////////////////

#define SWIZZLE2_DIM2()		\
	SWIZZLE2(xx, x, x)	\
	SWIZZLE2(xy, x, y)	\
	SWIZZLE2(yx, y, x)	\
	SWIZZLE2(yy, y, y)

#define SWIZZLE3_DIM2()		\
	SWIZZLE3(xxx, x, x, x)	\
	SWIZZLE3(xxy, x, x, y)	\
	SWIZZLE3(xyx, x, y, x)	\
	SWIZZLE3(xyy, x, y, y)	\
	SWIZZLE3(yxx, y, x, x)	\
	SWIZZLE3(yxy, y, x, y)	\
	SWIZZLE3(yyx, y, y, x)	\
	SWIZZLE3(yyy, y, y, y)

#define SWIZZLE4_DIM2()			\
	SWIZZLE4(xxxx, x, x, x, x)	\
	SWIZZLE4(xxxy, x, x, x, y)	\
	SWIZZLE4(xxyx, x, x, y, x)	\
	SWIZZLE4(xxyy, x, x, y, y)	\
	SWIZZLE4(xyxx, x, y, x, x)	\
	SWIZZLE4(xyxy, x, y, x, y)	\
	SWIZZLE4(xyyx, x, y, y, x)	\
	SWIZZLE4(xyyy, x, y, y, y)	\
	SWIZZLE4(yxxx, y, x, x, x)	\
	SWIZZLE4(yxxy, y, x, x, y)	\
	SWIZZLE4(yxyx, y, x, y, x)	\
	SWIZZLE4(yxyy, y, x, y, y)	\
	SWIZZLE4(yyxx, y, y, x, x)	\
	SWIZZLE4(yyxy, y, y, x, y)	\
	SWIZZLE4(yyyx, y, y, y, x)	\
	SWIZZLE4(yyyy, y, y, y, y)

#define SWIZZLE_EXPANSION_DIM2()	\
	SWIZZLE2_DIM2()			\
	SWIZZLE3_DIM2()			\
	SWIZZLE4_DIM2()

//////////////////////////////
// Three-dimensional vector //
//////////////////////////////

#define SWIZZLE2_DIM3()		\
	SWIZZLE2(xx, x, x)	\
	SWIZZLE2(xy, x, y)	\
	SWIZZLE2(xz, x, z)	\
	SWIZZLE2(yx, y, x)	\
	SWIZZLE2(yy, y, y)	\
	SWIZZLE2(yz, y, z)	\
	SWIZZLE2(zx, z, x)	\
	SWIZZLE2(zy, z, y)	\
	SWIZZLE2(zz, z, z)

#define SWIZZLE3_DIM3()			\
	SWIZZLE3(xxx, x, x, x);		\
	SWIZZLE3(xxy, x, x, y);		\
	SWIZZLE3(xxz, x, x, z);		\
	SWIZZLE3(xyx, x, y, x);		\
	SWIZZLE3(xyy, x, y, y);		\
	SWIZZLE3(xyz, x, y, z);		\
	SWIZZLE3(xzx, x, z, x);		\
	SWIZZLE3(xzy, x, z, y);		\
	SWIZZLE3(xzz, x, z, z);		\
	SWIZZLE3(yxx, y, x, x);		\
	SWIZZLE3(yxy, y, x, y);		\
	SWIZZLE3(yxz, y, x, z);		\
	SWIZZLE3(yyx, y, y, x);		\
	SWIZZLE3(yyy, y, y, y);		\
	SWIZZLE3(yyz, y, y, z);		\
	SWIZZLE3(yzx, y, z, x);		\
	SWIZZLE3(yzy, y, z, y);		\
	SWIZZLE3(yzz, y, z, z);		\
	SWIZZLE3(zxx, z, x, x);		\
	SWIZZLE3(zxy, z, x, y);		\
	SWIZZLE3(zxz, z, x, z);		\
	SWIZZLE3(zyx, z, y, x);		\
	SWIZZLE3(zyy, z, y, y);		\
	SWIZZLE3(zyz, z, y, z);		\
	SWIZZLE3(zzx, z, z, x);		\
	SWIZZLE3(zzy, z, z, y);		\
	SWIZZLE3(zzz, z, z, z);

#define SWIZZLE4_DIM3()			\
	SWIZZLE4(xxxx, x, x, x, x)	\
	SWIZZLE4(xxxy, x, x, x, y)	\
	SWIZZLE4(xxxz, x, x, x, z)	\
	SWIZZLE4(xxyx, x, x, y, x)	\
	SWIZZLE4(xxyy, x, x, y, y)	\
	SWIZZLE4(xxyz, x, x, y, z)	\
	SWIZZLE4(xxzx, x, x, z, x)	\
	SWIZZLE4(xxzy, x, x, z, y)	\
	SWIZZLE4(xxzz, x, x, z, z)	\
	SWIZZLE4(xyxx, x, y, x, x)	\
	SWIZZLE4(xyxy, x, y, x, y)	\
	SWIZZLE4(xyxz, x, y, x, z)	\
	SWIZZLE4(xyyx, x, y, y, x)	\
	SWIZZLE4(xyyy, x, y, y, y)	\
	SWIZZLE4(xyyz, x, y, y, z)	\
	SWIZZLE4(xyzx, x, y, z, x)	\
	SWIZZLE4(xyzy, x, y, z, y)	\
	SWIZZLE4(xyzz, x, y, z, z)	\
	SWIZZLE4(xzxx, x, z, x, x)	\
	SWIZZLE4(xzxy, x, z, x, y)	\
	SWIZZLE4(xzxz, x, z, x, z)	\
	SWIZZLE4(xzyx, x, z, y, x)	\
	SWIZZLE4(xzyy, x, z, y, y)	\
	SWIZZLE4(xzyz, x, z, y, z)	\
	SWIZZLE4(xzzx, x, z, z, x)	\
	SWIZZLE4(xzzy, x, z, z, y)	\
	SWIZZLE4(xzzz, x, z, z, z)	\
	SWIZZLE4(yxxx, y, x, x, x)	\
	SWIZZLE4(yxxy, y, x, x, y)	\
	SWIZZLE4(yxxz, y, x, x, z)	\
	SWIZZLE4(yxyx, y, x, y, x)	\
	SWIZZLE4(yxyy, y, x, y, y)	\
	SWIZZLE4(yxyz, y, x, y, z)	\
	SWIZZLE4(yxzx, y, x, z, x)	\
	SWIZZLE4(yxzy, y, x, z, y)	\
	SWIZZLE4(yxzz, y, x, z, z)	\
	SWIZZLE4(yyxx, y, y, x, x)	\
	SWIZZLE4(yyxy, y, y, x, y)	\
	SWIZZLE4(yyxz, y, y, x, z)	\
	SWIZZLE4(yyyx, y, y, y, x)	\
	SWIZZLE4(yyyy, y, y, y, y)	\
	SWIZZLE4(yyyz, y, y, y, z)	\
	SWIZZLE4(yyzx, y, y, z, x)	\
	SWIZZLE4(yyzy, y, y, z, y)	\
	SWIZZLE4(yyzz, y, y, z, z)	\
	SWIZZLE4(yzxx, y, z, x, x)	\
	SWIZZLE4(yzxy, y, z, x, y)	\
	SWIZZLE4(yzxz, y, z, x, z)	\
	SWIZZLE4(yzyx, y, z, y, x)	\
	SWIZZLE4(yzyy, y, z, y, y)	\
	SWIZZLE4(yzyz, y, z, y, z)	\
	SWIZZLE4(yzzx, y, z, z, x)	\
	SWIZZLE4(yzzy, y, z, z, y)	\
	SWIZZLE4(yzzz, y, z, z, z)	\
	SWIZZLE4(zxxx, z, x, x, x)	\
	SWIZZLE4(zxxy, z, x, x, y)	\
	SWIZZLE4(zxxz, z, x, x, z)	\
	SWIZZLE4(zxyx, z, x, y, x)	\
	SWIZZLE4(zxyy, z, x, y, y)	\
	SWIZZLE4(zxyz, z, x, y, z)	\
	SWIZZLE4(zxzx, z, x, z, x)	\
	SWIZZLE4(zxzy, z, x, z, y)	\
	SWIZZLE4(zxzz, z, x, z, z)	\
	SWIZZLE4(zyxx, z, y, x, x)	\
	SWIZZLE4(zyxy, z, y, x, y)	\
	SWIZZLE4(zyxz, z, y, x, z)	\
	SWIZZLE4(zyyx, z, y, y, x)	\
	SWIZZLE4(zyyy, z, y, y, y)	\
	SWIZZLE4(zyyz, z, y, y, z)	\
	SWIZZLE4(zyzx, z, y, z, x)	\
	SWIZZLE4(zyzy, z, y, z, y)	\
	SWIZZLE4(zyzz, z, y, z, z)	\
	SWIZZLE4(zzxx, z, z, x, x)	\
	SWIZZLE4(zzxy, z, z, x, y)	\
	SWIZZLE4(zzxz, z, z, x, z)	\
	SWIZZLE4(zzyx, z, z, y, x)	\
	SWIZZLE4(zzyy, z, z, y, y)	\
	SWIZZLE4(zzyz, z, z, y, z)	\
	SWIZZLE4(zzzx, z, z, z, x)	\
	SWIZZLE4(zzzy, z, z, z, y)	\
	SWIZZLE4(zzzz, z, z, z, z)

#define SWIZZLE_EXPANSION_DIM3()	\
	SWIZZLE2_DIM3()			\
	SWIZZLE3_DIM3()			\
	SWIZZLE4_DIM3()

/////////////////////////////
// Four-dimensional vector //
/////////////////////////////

#define SWIZZLE2_DIM4()		\
	SWIZZLE2(xx, x, x)	\
	SWIZZLE2(xy, x, y)	\
	SWIZZLE2(xz, x, z)	\
	SWIZZLE2(xw, x, w)	\
	SWIZZLE2(yx, y, x)	\
	SWIZZLE2(yy, y, y)	\
	SWIZZLE2(yz, y, z)	\
	SWIZZLE2(yw, y, w)	\
	SWIZZLE2(zx, z, x)	\
	SWIZZLE2(zy, z, y)	\
	SWIZZLE2(zz, z, z)	\
	SWIZZLE2(zw, z, w)	\
	SWIZZLE2(wx, w, x)	\
	SWIZZLE2(wy, w, y)	\
	SWIZZLE2(wz, w, z)	\
	SWIZZLE2(ww, w, w)

#define SWIZZLE3_DIM4()		\
	SWIZZLE3(xxx, x, x, x)	\
	SWIZZLE3(xxy, x, x, y)	\
	SWIZZLE3(xxz, x, x, z)	\
	SWIZZLE3(xxw, x, x, w)	\
	SWIZZLE3(xyx, x, y, x)	\
	SWIZZLE3(xyy, x, y, y)	\
	SWIZZLE3(xyz, x, y, z)	\
	SWIZZLE3(xyw, x, y, w)	\
	SWIZZLE3(xzx, x, z, x)	\
	SWIZZLE3(xzy, x, z, y)	\
	SWIZZLE3(xzz, x, z, z)	\
	SWIZZLE3(xzw, x, z, w)	\
	SWIZZLE3(xwx, x, w, x)	\
	SWIZZLE3(xwy, x, w, y)	\
	SWIZZLE3(xwz, x, w, z)	\
	SWIZZLE3(xww, x, w, w)	\
	SWIZZLE3(yxx, y, x, x)	\
	SWIZZLE3(yxy, y, x, y)	\
	SWIZZLE3(yxz, y, x, z)	\
	SWIZZLE3(yxw, y, x, w)	\
	SWIZZLE3(yyx, y, y, x)	\
	SWIZZLE3(yyy, y, y, y)	\
	SWIZZLE3(yyz, y, y, z)	\
	SWIZZLE3(yyw, y, y, w)	\
	SWIZZLE3(yzx, y, z, x)	\
	SWIZZLE3(yzy, y, z, y)	\
	SWIZZLE3(yzz, y, z, z)	\
	SWIZZLE3(yzw, y, z, w)	\
	SWIZZLE3(ywx, y, w, x)	\
	SWIZZLE3(ywy, y, w, y)	\
	SWIZZLE3(ywz, y, w, z)	\
	SWIZZLE3(yww, y, w, w)	\
	SWIZZLE3(zxx, z, x, x)	\
	SWIZZLE3(zxy, z, x, y)	\
	SWIZZLE3(zxz, z, x, z)	\
	SWIZZLE3(zxw, z, x, w)	\
	SWIZZLE3(zyx, z, y, x)	\
	SWIZZLE3(zyy, z, y, y)	\
	SWIZZLE3(zyz, z, y, z)	\
	SWIZZLE3(zyw, z, y, w)	\
	SWIZZLE3(zzx, z, z, x)	\
	SWIZZLE3(zzy, z, z, y)	\
	SWIZZLE3(zzz, z, z, z)	\
	SWIZZLE3(zzw, z, z, w)	\
	SWIZZLE3(zwx, z, w, x)	\
	SWIZZLE3(zwy, z, w, y)	\
	SWIZZLE3(zwz, z, w, z)	\
	SWIZZLE3(zww, z, w, w)	\
	SWIZZLE3(wxx, w, x, x)	\
	SWIZZLE3(wxy, w, x, y)	\
	SWIZZLE3(wxz, w, x, z)	\
	SWIZZLE3(wxw, w, x, w)	\
	SWIZZLE3(wyx, w, y, x)	\
	SWIZZLE3(wyy, w, y, y)	\
	SWIZZLE3(wyz, w, y, z)	\
	SWIZZLE3(wyw, w, y, w)	\
	SWIZZLE3(wzx, w, z, x)	\
	SWIZZLE3(wzy, w, z, y)	\
	SWIZZLE3(wzz, w, z, z)	\
	SWIZZLE3(wzw, w, z, w)	\
	SWIZZLE3(wwx, w, w, x)	\
	SWIZZLE3(wwy, w, w, y)	\
	SWIZZLE3(wwz, w, w, z)	\
	SWIZZLE3(www, w, w, w)

#define SWIZZLE4_DIM4()			\
	SWIZZLE4(xxxx, x, x, x, x)	\
	SWIZZLE4(xxxy, x, x, x, y)	\
	SWIZZLE4(xxxz, x, x, x, z)	\
	SWIZZLE4(xxxw, x, x, x, w)	\
	SWIZZLE4(xxyx, x, x, y, x)	\
	SWIZZLE4(xxyy, x, x, y, y)	\
	SWIZZLE4(xxyz, x, x, y, z)	\
	SWIZZLE4(xxyw, x, x, y, w)	\
	SWIZZLE4(xxzx, x, x, z, x)	\
	SWIZZLE4(xxzy, x, x, z, y)	\
	SWIZZLE4(xxzz, x, x, z, z)	\
	SWIZZLE4(xxzw, x, x, z, w)	\
	SWIZZLE4(xxwx, x, x, w, x)	\
	SWIZZLE4(xxwy, x, x, w, y)	\
	SWIZZLE4(xxwz, x, x, w, z)	\
	SWIZZLE4(xxww, x, x, w, w)	\
	SWIZZLE4(xyxx, x, y, x, x)	\
	SWIZZLE4(xyxy, x, y, x, y)	\
	SWIZZLE4(xyxz, x, y, x, z)	\
	SWIZZLE4(xyxw, x, y, x, w)	\
	SWIZZLE4(xyyx, x, y, y, x)	\
	SWIZZLE4(xyyy, x, y, y, y)	\
	SWIZZLE4(xyyz, x, y, y, z)	\
	SWIZZLE4(xyyw, x, y, y, w)	\
	SWIZZLE4(xyzx, x, y, z, x)	\
	SWIZZLE4(xyzy, x, y, z, y)	\
	SWIZZLE4(xyzz, x, y, z, z)	\
	SWIZZLE4(xyzw, x, y, z, w)	\
	SWIZZLE4(xywx, x, y, w, x)	\
	SWIZZLE4(xywy, x, y, w, y)	\
	SWIZZLE4(xywz, x, y, w, z)	\
	SWIZZLE4(xyww, x, y, w, w)	\
	SWIZZLE4(xzxx, x, z, x, x)	\
	SWIZZLE4(xzxy, x, z, x, y)	\
	SWIZZLE4(xzxz, x, z, x, z)	\
	SWIZZLE4(xzxw, x, z, x, w)	\
	SWIZZLE4(xzyx, x, z, y, x)	\
	SWIZZLE4(xzyy, x, z, y, y)	\
	SWIZZLE4(xzyz, x, z, y, z)	\
	SWIZZLE4(xzyw, x, z, y, w)	\
	SWIZZLE4(xzzx, x, z, z, x)	\
	SWIZZLE4(xzzy, x, z, z, y)	\
	SWIZZLE4(xzzz, x, z, z, z)	\
	SWIZZLE4(xzzw, x, z, z, w)	\
	SWIZZLE4(xzwx, x, z, w, x)	\
	SWIZZLE4(xzwy, x, z, w, y)	\
	SWIZZLE4(xzwz, x, z, w, z)	\
	SWIZZLE4(xzww, x, z, w, w)	\
	SWIZZLE4(xwxx, x, w, x, x)	\
	SWIZZLE4(xwxy, x, w, x, y)	\
	SWIZZLE4(xwxz, x, w, x, z)	\
	SWIZZLE4(xwxw, x, w, x, w)	\
	SWIZZLE4(xwyx, x, w, y, x)	\
	SWIZZLE4(xwyy, x, w, y, y)	\
	SWIZZLE4(xwyz, x, w, y, z)	\
	SWIZZLE4(xwyw, x, w, y, w)	\
	SWIZZLE4(xwzx, x, w, z, x)	\
	SWIZZLE4(xwzy, x, w, z, y)	\
	SWIZZLE4(xwzz, x, w, z, z)	\
	SWIZZLE4(xwzw, x, w, z, w)	\
	SWIZZLE4(xwwx, x, w, w, x)	\
	SWIZZLE4(xwwy, x, w, w, y)	\
	SWIZZLE4(xwwz, x, w, w, z)	\
	SWIZZLE4(xwww, x, w, w, w)	\
	SWIZZLE4(yxxx, y, x, x, x)	\
	SWIZZLE4(yxxy, y, x, x, y)	\
	SWIZZLE4(yxxz, y, x, x, z)	\
	SWIZZLE4(yxxw, y, x, x, w)	\
	SWIZZLE4(yxyx, y, x, y, x)	\
	SWIZZLE4(yxyy, y, x, y, y)	\
	SWIZZLE4(yxyz, y, x, y, z)	\
	SWIZZLE4(yxyw, y, x, y, w)	\
	SWIZZLE4(yxzx, y, x, z, x)	\
	SWIZZLE4(yxzy, y, x, z, y)	\
	SWIZZLE4(yxzz, y, x, z, z)	\
	SWIZZLE4(yxzw, y, x, z, w)	\
	SWIZZLE4(yxwx, y, x, w, x)	\
	SWIZZLE4(yxwy, y, x, w, y)	\
	SWIZZLE4(yxwz, y, x, w, z)	\
	SWIZZLE4(yxww, y, x, w, w)	\
	SWIZZLE4(yyxx, y, y, x, x)	\
	SWIZZLE4(yyxy, y, y, x, y)	\
	SWIZZLE4(yyxz, y, y, x, z)	\
	SWIZZLE4(yyxw, y, y, x, w)	\
	SWIZZLE4(yyyx, y, y, y, x)	\
	SWIZZLE4(yyyy, y, y, y, y)	\
	SWIZZLE4(yyyz, y, y, y, z)	\
	SWIZZLE4(yyyw, y, y, y, w)	\
	SWIZZLE4(yyzx, y, y, z, x)	\
	SWIZZLE4(yyzy, y, y, z, y)	\
	SWIZZLE4(yyzz, y, y, z, z)	\
	SWIZZLE4(yyzw, y, y, z, w)	\
	SWIZZLE4(yywx, y, y, w, x)	\
	SWIZZLE4(yywy, y, y, w, y)	\
	SWIZZLE4(yywz, y, y, w, z)	\
	SWIZZLE4(yyww, y, y, w, w)	\
	SWIZZLE4(yzxx, y, z, x, x)	\
	SWIZZLE4(yzxy, y, z, x, y)	\
	SWIZZLE4(yzxz, y, z, x, z)	\
	SWIZZLE4(yzxw, y, z, x, w)	\
	SWIZZLE4(yzyx, y, z, y, x)	\
	SWIZZLE4(yzyy, y, z, y, y)	\
	SWIZZLE4(yzyz, y, z, y, z)	\
	SWIZZLE4(yzyw, y, z, y, w)	\
	SWIZZLE4(yzzx, y, z, z, x)	\
	SWIZZLE4(yzzy, y, z, z, y)	\
	SWIZZLE4(yzzz, y, z, z, z)	\
	SWIZZLE4(yzzw, y, z, z, w)	\
	SWIZZLE4(yzwx, y, z, w, x)	\
	SWIZZLE4(yzwy, y, z, w, y)	\
	SWIZZLE4(yzwz, y, z, w, z)	\
	SWIZZLE4(yzww, y, z, w, w)	\
	SWIZZLE4(ywxx, y, w, x, x)	\
	SWIZZLE4(ywxy, y, w, x, y)	\
	SWIZZLE4(ywxz, y, w, x, z)	\
	SWIZZLE4(ywxw, y, w, x, w)	\
	SWIZZLE4(ywyx, y, w, y, x)	\
	SWIZZLE4(ywyy, y, w, y, y)	\
	SWIZZLE4(ywyz, y, w, y, z)	\
	SWIZZLE4(ywyw, y, w, y, w)	\
	SWIZZLE4(ywzx, y, w, z, x)	\
	SWIZZLE4(ywzy, y, w, z, y)	\
	SWIZZLE4(ywzz, y, w, z, z)	\
	SWIZZLE4(ywzw, y, w, z, w)	\
	SWIZZLE4(ywwx, y, w, w, x)	\
	SWIZZLE4(ywwy, y, w, w, y)	\
	SWIZZLE4(ywwz, y, w, w, z)	\
	SWIZZLE4(ywww, y, w, w, w)	\
	SWIZZLE4(zxxx, z, x, x, x)	\
	SWIZZLE4(zxxy, z, x, x, y)	\
	SWIZZLE4(zxxz, z, x, x, z)	\
	SWIZZLE4(zxxw, z, x, x, w)	\
	SWIZZLE4(zxyx, z, x, y, x)	\
	SWIZZLE4(zxyy, z, x, y, y)	\
	SWIZZLE4(zxyz, z, x, y, z)	\
	SWIZZLE4(zxyw, z, x, y, w)	\
	SWIZZLE4(zxzx, z, x, z, x)	\
	SWIZZLE4(zxzy, z, x, z, y)	\
	SWIZZLE4(zxzz, z, x, z, z)	\
	SWIZZLE4(zxzw, z, x, z, w)	\
	SWIZZLE4(zxwx, z, x, w, x)	\
	SWIZZLE4(zxwy, z, x, w, y)	\
	SWIZZLE4(zxwz, z, x, w, z)	\
	SWIZZLE4(zxww, z, x, w, w)	\
	SWIZZLE4(zyxx, z, y, x, x)	\
	SWIZZLE4(zyxy, z, y, x, y)	\
	SWIZZLE4(zyxz, z, y, x, z)	\
	SWIZZLE4(zyxw, z, y, x, w)	\
	SWIZZLE4(zyyx, z, y, y, x)	\
	SWIZZLE4(zyyy, z, y, y, y)	\
	SWIZZLE4(zyyz, z, y, y, z)	\
	SWIZZLE4(zyyw, z, y, y, w)	\
	SWIZZLE4(zyzx, z, y, z, x)	\
	SWIZZLE4(zyzy, z, y, z, y)	\
	SWIZZLE4(zyzz, z, y, z, z)	\
	SWIZZLE4(zyzw, z, y, z, w)	\
	SWIZZLE4(zywx, z, y, w, x)	\
	SWIZZLE4(zywy, z, y, w, y)	\
	SWIZZLE4(zywz, z, y, w, z)	\
	SWIZZLE4(zyww, z, y, w, w)	\
	SWIZZLE4(zzxx, z, z, x, x)	\
	SWIZZLE4(zzxy, z, z, x, y)	\
	SWIZZLE4(zzxz, z, z, x, z)	\
	SWIZZLE4(zzxw, z, z, x, w)	\
	SWIZZLE4(zzyx, z, z, y, x)	\
	SWIZZLE4(zzyy, z, z, y, y)	\
	SWIZZLE4(zzyz, z, z, y, z)	\
	SWIZZLE4(zzyw, z, z, y, w)	\
	SWIZZLE4(zzzx, z, z, z, x)	\
	SWIZZLE4(zzzy, z, z, z, y)	\
	SWIZZLE4(zzzz, z, z, z, z)	\
	SWIZZLE4(zzzw, z, z, z, w)	\
	SWIZZLE4(zzwx, z, z, w, x)	\
	SWIZZLE4(zzwy, z, z, w, y)	\
	SWIZZLE4(zzwz, z, z, w, z)	\
	SWIZZLE4(zzww, z, z, w, w)	\
	SWIZZLE4(zwxx, z, w, x, x)	\
	SWIZZLE4(zwxy, z, w, x, y)	\
	SWIZZLE4(zwxz, z, w, x, z)	\
	SWIZZLE4(zwxw, z, w, x, w)	\
	SWIZZLE4(zwyx, z, w, y, x)	\
	SWIZZLE4(zwyy, z, w, y, y)	\
	SWIZZLE4(zwyz, z, w, y, z)	\
	SWIZZLE4(zwyw, z, w, y, w)	\
	SWIZZLE4(zwzx, z, w, z, x)	\
	SWIZZLE4(zwzy, z, w, z, y)	\
	SWIZZLE4(zwzz, z, w, z, z)	\
	SWIZZLE4(zwzw, z, w, z, w)	\
	SWIZZLE4(zwwx, z, w, w, x)	\
	SWIZZLE4(zwwy, z, w, w, y)	\
	SWIZZLE4(zwwz, z, w, w, z)	\
	SWIZZLE4(zwww, z, w, w, w)	\
	SWIZZLE4(wxxx, w, x, x, x)	\
	SWIZZLE4(wxxy, w, x, x, y)	\
	SWIZZLE4(wxxz, w, x, x, z)	\
	SWIZZLE4(wxxw, w, x, x, w)	\
	SWIZZLE4(wxyx, w, x, y, x)	\
	SWIZZLE4(wxyy, w, x, y, y)	\
	SWIZZLE4(wxyz, w, x, y, z)	\
	SWIZZLE4(wxyw, w, x, y, w)	\
	SWIZZLE4(wxzx, w, x, z, x)	\
	SWIZZLE4(wxzy, w, x, z, y)	\
	SWIZZLE4(wxzz, w, x, z, z)	\
	SWIZZLE4(wxzw, w, x, z, w)	\
	SWIZZLE4(wxwx, w, x, w, x)	\
	SWIZZLE4(wxwy, w, x, w, y)	\
	SWIZZLE4(wxwz, w, x, w, z)	\
	SWIZZLE4(wxww, w, x, w, w)	\
	SWIZZLE4(wyxx, w, y, x, x)	\
	SWIZZLE4(wyxy, w, y, x, y)	\
	SWIZZLE4(wyxz, w, y, x, z)	\
	SWIZZLE4(wyxw, w, y, x, w)	\
	SWIZZLE4(wyyx, w, y, y, x)	\
	SWIZZLE4(wyyy, w, y, y, y)	\
	SWIZZLE4(wyyz, w, y, y, z)	\
	SWIZZLE4(wyyw, w, y, y, w)	\
	SWIZZLE4(wyzx, w, y, z, x)	\
	SWIZZLE4(wyzy, w, y, z, y)	\
	SWIZZLE4(wyzz, w, y, z, z)	\
	SWIZZLE4(wyzw, w, y, z, w)	\
	SWIZZLE4(wywx, w, y, w, x)	\
	SWIZZLE4(wywy, w, y, w, y)	\
	SWIZZLE4(wywz, w, y, w, z)	\
	SWIZZLE4(wyww, w, y, w, w)	\
	SWIZZLE4(wzxx, w, z, x, x)	\
	SWIZZLE4(wzxy, w, z, x, y)	\
	SWIZZLE4(wzxz, w, z, x, z)	\
	SWIZZLE4(wzxw, w, z, x, w)	\
	SWIZZLE4(wzyx, w, z, y, x)	\
	SWIZZLE4(wzyy, w, z, y, y)	\
	SWIZZLE4(wzyz, w, z, y, z)	\
	SWIZZLE4(wzyw, w, z, y, w)	\
	SWIZZLE4(wzzx, w, z, z, x)	\
	SWIZZLE4(wzzy, w, z, z, y)	\
	SWIZZLE4(wzzz, w, z, z, z)	\
	SWIZZLE4(wzzw, w, z, z, w)	\
	SWIZZLE4(wzwx, w, z, w, x)	\
	SWIZZLE4(wzwy, w, z, w, y)	\
	SWIZZLE4(wzwz, w, z, w, z)	\
	SWIZZLE4(wzww, w, z, w, w)	\
	SWIZZLE4(wwxx, w, w, x, x)	\
	SWIZZLE4(wwxy, w, w, x, y)	\
	SWIZZLE4(wwxz, w, w, x, z)	\
	SWIZZLE4(wwxw, w, w, x, w)	\
	SWIZZLE4(wwyx, w, w, y, x)	\
	SWIZZLE4(wwyy, w, w, y, y)	\
	SWIZZLE4(wwyz, w, w, y, z)	\
	SWIZZLE4(wwyw, w, w, y, w)	\
	SWIZZLE4(wwzx, w, w, z, x)	\
	SWIZZLE4(wwzy, w, w, z, y)	\
	SWIZZLE4(wwzz, w, w, z, z)	\
	SWIZZLE4(wwzw, w, w, z, w)	\
	SWIZZLE4(wwwx, w, w, w, x)	\
	SWIZZLE4(wwwy, w, w, w, y)	\
	SWIZZLE4(wwwz, w, w, w, z)	\
	SWIZZLE4(wwww, w, w, w, w)

#define SWIZZLE_EXPANSION_DIM4()	\
	SWIZZLE2_DIM4()			\
	SWIZZLE3_DIM4()			\
	SWIZZLE4_DIM4()
