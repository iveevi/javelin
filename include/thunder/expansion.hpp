#pragma once

#include "buffer.hpp"

namespace jvl::thunder {

struct Expansion : Buffer {
	Index value;

	// local index -> mask of local addresses
	//     mask = 0bXY
	//     X is mask (on=local) for 1st arg
	//     Y is mask (on=local) for 2nd arg 
	std::map <Index, uint8_t> locals;

	void returns(Index);
	void track(Index, uint8_t = 0b11);
};

struct Mapped : std::map <Index, Expansion> {
	Buffer &base;

	Mapped(Buffer &);

	void display() const;
	Buffer stitch() const;
};

} // namespace jvl::thunder