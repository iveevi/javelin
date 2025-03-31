#pragma once

#include "buffer.hpp"

namespace jvl::thunder {

struct Legalizer {
	// TODO: flags...

	void storage(Buffer &);

	static Legalizer stable;
};

} // namespace jvl::thunder