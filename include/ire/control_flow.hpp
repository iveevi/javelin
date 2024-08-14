#pragma once

#include "aliases.hpp"
#include "atom/atom.hpp"
#include "type_synthesis.hpp"

namespace jvl::ire {

inline void cond(const boolean &b)
{
	auto &em = Emitter::active;
	atom::Cond branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

inline void elif(const boolean &b)
{
	auto &em = Emitter::active;
	atom::Elif branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

inline void elif()
{
	// Treated as an else
	auto &em = Emitter::active;
	atom::Elif branch;
	branch.cond = -1;
	em.emit_main(branch);
}

inline void loop(const boolean &b)
{
	auto &em = Emitter::active;
	atom::While branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

template <typename ... Args>
inline void returns(const Args &... args)
{
	auto &em = Emitter::active;
	atom::Returns ret;
	if constexpr (sizeof...(Args)) {
		ret.args = list_from_args(args...);
		ret.type = type_field_from_args <std::decay_t <Args>...> ().id;
	} else {
		atom::TypeField tf;
		tf.item = atom::none;
		ret.type = em.emit(tf);
	}

	em.emit_main(ret);
}

inline void end()
{
	auto &em = Emitter::active;
	em.emit_main(atom::End());
}

} // namespace jvl::ire
