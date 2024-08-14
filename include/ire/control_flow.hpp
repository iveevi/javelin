#pragma once

#include "aliases.hpp"

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
	em.mark_synthesized_underlying(branch.cond);
}

inline void returns()
{
	auto &em = Emitter::active;
	atom::Returns ret;
	em.emit_main(ret);
}

inline void end()
{
	auto &em = Emitter::active;
	em.emit_main(atom::End());
}

} // namespace jvl::ire
