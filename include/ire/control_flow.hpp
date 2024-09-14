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
	Emitter::active.emit_return(
		native_t <T> (value).synthesize().id,
		type_field_from_args <T> ().id);
}

template <builtin T>
inline void returns(const T &value)
{
	Emitter::active.emit_return(value.synthesize().id,
		type_field_from_args <T> ().id);
}

template <aggregate T>
inline void returns(const T &value)
{
	auto &em = Emitter::active;
	auto layout = value.layout();
	cache_index_t args = layout.list();
	thunder::index_t type = type_field_from_args(layout).id;
	thunder::index_t rv = em.emit_construct(type, args.id, false);
	em.emit_return(rv, type);
}

} // namespace jvl::ire
