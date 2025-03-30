#pragma once

#include "../thunder/enumerations.hpp"
#include "aliases.hpp"
#include "tagged.hpp"

namespace jvl::ire {

// Any function beginning with $ is a control-flow related
// operation specific to the Javelin IRE system

inline void $if(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_if;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void $elif(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_else_if;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void $else()
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_else;
	branch.cond = -1;
	em.emit(branch);
}

inline void $while(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::loop_while;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

// Python-like range loops
template <builtin T>
struct _range {
	T start;
	T end;
	T step;
};

// Smoother conversion from native types and the like
template <integral_arithmetic T, integral_arithmetic U, integral_arithmetic V = T>
auto range(const T &start, const U &end, const V &step = V(1))
{
	using result = decltype(underlying(start));

	return _range <result> {
		underlying(start),
		underlying(end),
		underlying(step)
	};
}

// Imperative-style for loops... see the bottom for syntactic sugar
template <builtin T>
inline T _for(const _range <T> &range)
{
	auto &em = Emitter::active;

	T i = clone(range.start);

	boolean cond = i < range.end;

	auto pre = [i, range]() {
		T l = i;
		l += range.step;
	};

	em.emit_branch(cond.synthesize().id, -1, thunder::loop_while, pre);

	return i;
}

[[gnu::always_inline]]
inline void $break()
{
	Emitter::active.emit_branch(-1, -1, thunder::control_flow_stop);
}

[[gnu::always_inline]]
inline void $continue()
{
	Emitter::active.emit_branch(-1, -1, thunder::control_flow_skip);
}

[[gnu::always_inline]]
inline void $end()
{
	Emitter::active.emit_branch(-1, -1, thunder::control_flow_end);
}

template <builtin T>
inline T $select(const boolean &b, const T &vt, const T &vf)
{
	T v = vf;
	$if (b);
		v = vt;
	$end();
	return v;
}

template <native T>
inline native_t <T> $select(const boolean &b, const T &vt, const T &vf)
{
	auto nvt = native_t <T> (vt);
	auto nvf = native_t <T> (vf);
	return $select(b, nvt, nvf);
}

// TODO: match/match_case statements
inline void $return()
{
	Emitter::active.emit_return(-1);
}

template <native T>
inline void $return(const T &value)
{
	auto nvalue = native_t <T> (value);
	Emitter::active.emit_return(nvalue.synthesize().id);
}

template <builtin T>
inline void $return(const T &value)
{
	Emitter::active.emit_return(value.synthesize().id);
}

template <aggregate T>
inline void $return(T &value)
{
	auto &em = Emitter::active;
	auto layout = value.layout();
	auto type = layout.generate_type().concrete();
	auto args = layout.reconstruct();
	auto rv = em.emit_construct(type, args, thunder::normal);
	em.emit_return(rv);
}

// Macros for syntactic sugar
template <builtin T>
struct _for_igniter {
	_range <T> it;

	void operator<<(const std::function <void (T)> &hold) {
		auto i = _for(it);
			hold(i);
		$end();
	}
};

#define $for(id, it) _for_igniter(it) << [&](auto id) -> void

} // namespace jvl::ire
