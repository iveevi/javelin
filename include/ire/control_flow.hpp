#pragma once

#include "aliases.hpp"
#include "ire/tagged.hpp"
#include "ire/uniform_layout.hpp"
#include "logging.hpp"
#include "thunder/enumerations.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

inline void cond(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_if;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void elif(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_else_if;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void elif()
{
	// Treated as an else
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::conditional_else;
	branch.cond = -1;
	em.emit(branch);
}

inline void loop(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::loop_while;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

// Python-like range loops
template <builtin T>
struct range {
	T start;
	T end;
	T step;

	range(const T &start_, const T &end_, const T &step_ = T(1))
		: start(start_), end(end_), step(step_) {}
};

template <builtin T>
inline T loop(const range <T>  &range)
{
	auto &em = Emitter::active;

	T i = range.start;
	boolean cond = i < range.end;
	
	auto pre = [i, range](){
		T l = i;
		l += range.step;
	};

	em.emit_branch(cond.synthesize().id, -1, thunder::loop_while, pre);

	return i;
}

inline void end()
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.kind = thunder::control_flow_end;
	em.emit(branch);
}

// TODO: match/match_case statements
template <native T>
inline void returns(const T &value)
{
	Emitter::active.emit_return(native_t <T> (value).synthesize().id);
}

template <builtin T>
inline void returns(const T &value)
{
	Emitter::active.emit_return(value.synthesize().id);
}

template <aggregate T>
inline void returns(const T &value)
{
	auto &em = Emitter::active;
	auto layout = value.layout();
	cache_index_t args = layout.list();
	thunder::index_t type = type_field_from_args(layout).id;
	thunder::index_t rv = em.emit_construct(type, args.id, false);
	em.emit_return(rv);
}

} // namespace jvl::ire
