#pragma once

#include "aliases.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

inline void cond(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void elif(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

inline void elif()
{
	// Treated as an else
	auto &em = Emitter::active;
	thunder::Branch branch;
	branch.cond = -1;
	em.emit(branch);
}

inline void loop(const boolean &b)
{
	auto &em = Emitter::active;
	thunder::While branch;
	branch.cond = b.synthesize().id;
	em.emit(branch);
}

template <typename ... Args>
inline void returns(const Args &... args)
{
	auto &em = Emitter::active;
	thunder::Returns ret;
	if constexpr (sizeof...(Args)) {
		ret.args = list_from_args(args...);
		ret.type = type_field_from_args <std::decay_t <Args>...> ().id;
	} else {
		thunder::TypeField tf;
		tf.item = thunder::none;
		ret.type = em.emit(tf);
	}

	em.emit(ret);
}

inline void end()
{
	auto &em = Emitter::active;
	em.emit(thunder::End());
}

} // namespace jvl::ire
