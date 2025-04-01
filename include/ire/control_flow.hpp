#pragma once

#include "../thunder/enumerations.hpp"
#include "aliases.hpp"
#include "tagged.hpp"

namespace jvl::ire {

inline void _if(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_if;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void _elif(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_else_if;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void _else()
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_else;
	branch.cond = -1;
	em.emit(branch);
}

inline void _while(const boolean &b)
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
inline void _break()
{
	Emitter::active.emit_branch(-1, -1, thunder::control_flow_stop);
}

[[gnu::always_inline]]
inline void _continue()
{
	Emitter::active.emit_branch(-1, -1, thunder::control_flow_skip);
}

[[gnu::always_inline]]
inline void _end()
{
	Emitter::active.emit_branch(-1, -1, thunder::control_flow_end);
}

// TODO: match/match_case statements

template <builtin T>
inline T $select(const boolean &b, const T &vt, const T &vf)
{
	T v = vf;
	_if (b);
		v = vt;
	_end();
	return v;
}

template <native T>
inline native_t <T> $select(const boolean &b, const T &vt, const T &vf)
{
	auto nvt = native_t <T> (vt);
	auto nvf = native_t <T> (vf);
	return $select(b, nvt, nvf);
}

// Returning values
inline void _return()
{
	Emitter::active.emit_return(-1);
}

template <native T>
inline void _return(const T &value)
{
	auto nvalue = native_t <T> (value);
	Emitter::active.emit_return(nvalue.synthesize().id);
}

template <builtin T>
inline void _return(const T &value)
{
	Emitter::active.emit_return(value.synthesize().id);
}

template <aggregate T>
inline void _return(T &value)
{
	auto &em = Emitter::active;
	auto layout = value.layout();
	auto type = layout.generate_type().concrete();
	auto args = layout.reconstruct();
	auto rv = em.emit_construct(type, args, thunder::normal);
	em.emit_return(rv);
}

////////////////////////////////
// Macros for syntactic sugar //
////////////////////////////////

// Branch conditions
using branch_body = std::function <void ()>;

struct _if_holder {
	boolean cond;
	branch_body handle = nullptr;

	template <typename F>
	_if_holder operator*(const F &passed) const {
		return _if_holder(cond, std::function(passed));
	}
};

struct _else_if_holder {
	boolean cond;
	branch_body handle = nullptr;

	template <typename F>
	_else_if_holder operator*(const F &passed) const {
		return _else_if_holder(cond, std::function(passed));
	}
};

struct _else_holder {
	branch_body handle = nullptr;

	template <typename F>
	_else_holder operator*(const F &passed) const {
		return _else_holder(std::function(passed));
	}
};

// TODO: template bool on whether the end has been reached
struct _branch_holder {
	std::optional <_if_holder>    hif;
	std::optional <_else_holder>  helse;
	std::vector <_else_if_holder> helifs;

	_branch_holder() = default;

	_branch_holder(const _branch_holder &) = delete;
	_branch_holder &operator=(const _branch_holder &) = delete;

	_branch_holder(_branch_holder &&other) {
		hif = other.hif;
		helifs = other.helifs;
		helse = other.helse;

		other.hif.reset();
		other.helifs.clear();
		other.helse.reset();
	}

	~_branch_holder() {
		if (!hif.has_value())
			return;

		// Generate full branch source
		auto _ifv = hif.value();

		_if(_ifv.cond);
			_ifv.handle();

		for (auto &_elifv : helifs) {
			_elif(_elifv.cond);
				_elifv.handle();	
		}

		if (helse) {
			auto _elsev = helse.value();

			_else();
				_elsev.handle();
		}
		
		_end();
	}
};

// TODO: move to source
inline _branch_holder operator+(_branch_holder &&body, const _if_holder &_if)
{
	auto result = std::move(body);
	result.hif = _if;
	return result;
}

inline _branch_holder operator+(_branch_holder &&body, const _else_if_holder &_elif)
{
	auto result = std::move(body);
	result.helifs.emplace_back(_elif);
	return result;
}

inline _branch_holder operator+(_branch_holder &&body, const _else_holder &_else)
{
	auto result = std::move(body);
	result.helse = _else;
	return result;
}

#define $if(cond)	_branch_holder() + _if_holder(cond) * [&]()
#define $elif(cond)	+ _else_if_holder(cond) * [&]()
#define $else		+ _else_holder() * [&]()

// For loops
template <builtin T>
struct _for_igniter {
	_range <T> it;

	void operator<<(const std::function <void (T)> &hold) {
		auto i = _for(it);
			hold(i);
		_end();
	}
};

#define $for(id, it)	_for_igniter(it) << [&](auto id) -> void
#define $continue	_continue()
#define $break		_break()

// Returns
struct _void {};

struct _return_igniter {
	void operator<<(auto value) const {
		using T = decltype(value);

		if constexpr (std::same_as <T, _void>)
			return _return();
		else
			return _return(value);
	}
};

#define $return	_return_igniter() <<
#define $void	_void()

} // namespace jvl::ire
