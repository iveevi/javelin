#pragma once

#include "buffer.hpp"
#include "tracked_buffer.hpp"
#include "usage.hpp"

namespace jvl::thunder {

// Optimization transformation passes
bool optimize_dead_code_elimination_iteration(Buffer &);
bool optimize_dead_code_elimination(Buffer &);

bool optimize_deduplicate_iteration(Buffer &);
bool optimize_deduplicate(Buffer &);

bool optimize_casting_elision(Buffer &);

// Full optimization pass
void optimize(Buffer &);
void optimize(TrackedBuffer &);

// Legalizing instructions for C-family compiled targets
void legalize_for_cc(Buffer &);

// Stitching mapped instruction blocks
void stitch_mapped_instructions(Buffer &, std::vector <mapped_instruction_t> &);

} // namespace jvl::thunder
