#pragma once

#include "aliases.hpp"

namespace jvl::ire {

inline void cond(const boolean &b)
{
	auto &em = Emitter::active;
	op::Cond branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

inline void elif(const boolean &b)
{
	auto &em = Emitter::active;
	op::Elif branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
}

inline void elif()
{
	// Treated as an else
	auto &em = Emitter::active;
	op::Elif branch;
	branch.cond = -1;
	em.emit_main(branch);
}

inline void loop(const boolean &b)
{
	auto &em = Emitter::active;
	op::While branch;
	branch.cond = b.synthesize().id;
	em.emit_main(branch);
	em.mark_synthesized_underlying(branch.cond);
}

inline void end()
{
	auto &em = Emitter::active;
	em.emit_main(op::End());
}

} // namespace jvl::ire
