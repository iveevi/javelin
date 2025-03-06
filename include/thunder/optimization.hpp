#pragma once

#include "../common/flags.hpp"

#include "buffer.hpp"
#include "tracked_buffer.hpp"
#include "usage.hpp"

namespace jvl::thunder {

// Utility
void refine_relocation(reindex <Index> &);
void buffer_relocation(Buffer &, const reindex <Index> &);

// Flags for optimization passes
enum class OptimizationFlags : uint8_t {
	eNone = 0b0,
	eDeadCodeElimination = 0b1,
	eDeduplication = 0b10,
	eCastingElision = 0b100,
	eStoreElision = 0b1000,
	eStable = eDeadCodeElimination
		| eDeduplication
		| eCastingElision,
	eAll = eDeadCodeElimination
		| eDeduplication
		| eCastingElision
		| eStoreElision,
};

flag_operators(OptimizationFlags, uint8_t);

// Optimization transformation passes
bool optimize_dead_code_elimination_iteration(Buffer &);
bool optimize_dead_code_elimination(Buffer &);

uint32_t optimize_deduplicate_iteration(Buffer &);
bool optimize_deduplicate(Buffer &);

bool optimize_casting_elision(Buffer &);

bool optimize_store_elision(Buffer &);

// Full optimization pass
void optimize(Buffer &, const OptimizationFlags = OptimizationFlags::eStable);
void optimize(TrackedBuffer &, const OptimizationFlags = OptimizationFlags::eStable);

// Legalizing instructions for C-family compiled targets
void legalize_for_cc(Buffer &);

// Stitching mapped instruction blocks
void stitch_mapped_instructions(Buffer &, std::vector <mapped_instruction_t> &);

} // namespace jvl::thunder
