#pragma once

#include "aliases.hpp"
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

template <typename ... Args>
inline void returns(const Args &... args)
{
	auto &em = Emitter::active;
	thunder::Returns ret;
	if constexpr (sizeof...(Args)) {
		ret.args = list_from_args(args...);
		ret.type = type_field_from_args <std::decay_t <Args>...> ().id;
	} else {
		thunder::TypeInformation tf;
		tf.item = thunder::none;
		ret.type = em.emit(tf);
	}

	em.emit(ret);
}

} // namespace jvl::ire
