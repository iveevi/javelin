#pragma once

#include "atom.hpp"
#include "buffer.hpp"

namespace jvl::thunder {

struct Relocation : std::map <Index, Index> {
	void apply(Index &addr) const {
		auto it = find(addr);
		if (addr != -1 && it != end())
			addr = it->second;
	}

	void apply(Atom &atom) const {
		auto addrs = atom.addresses();

		apply(addrs.a0);
		apply(addrs.a1);
	}

	void apply(Buffer &buffer) const {
		for (size_t i = 0; i < buffer.pointer; i++)
			apply(buffer.atoms[i]);
	}
};

} // namespace jvl::thunder