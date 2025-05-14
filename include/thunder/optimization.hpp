#pragma once

#include "../common/flags.hpp"

#include "buffer.hpp"
#include "relocation.hpp"
#include "tracked_buffer.hpp"
#include "usage.hpp"

namespace jvl::thunder {

// Flags for optimization passes
enum class OptimizationFlags : uint8_t {
	eNone			= 0b0,
	eDeadCodeElimination	= 0b1,
	eDeduplication		= 0b10,
	eCastingElision		= 0b100,
	eStoreElision		= 0b1000,
	eStable			= eDeadCodeElimination
				| eCastingElision,
	eAll			= eDeadCodeElimination
				| eDeduplication
				| eCastingElision
				| eStoreElision,
};

DEFINE_FLAG_OPERATORS(OptimizationFlags, uint8_t);

// Flags for disolve optimization pass
enum class DisolveFlags : uint8_t {
	eNone			= 0b0,
	eCasting		= 0b1,
	eStores			= 0b10,
	eAll			= eCasting | eStores,
};

DEFINE_FLAG_OPERATORS(DisolveFlags, uint8_t);

// Legalizing instructions for C-family compiled targets
void legalize_for_cc(Buffer &);

// Stitching mapped instruction blocks
void stitch_mapped_instructions(Buffer &, std::vector <mapped_instruction_t> &);

// Optimizer class
struct Optimizer {
	using HashMap = std::map <uint64_t, Index>;

	OptimizationFlags flags;

	// Instruction distillation (deduplication)
	uint32_t distill_types_once(Buffer &) const;
	void distill_types(Buffer &) const;
	void distill(Buffer &) const;

	// Instruction disolving (removal and elision)
	bool disolve_casting_intrinsic(Relocation &, const Buffer &, const Intrinsic &, Index) const;
	bool disolve_casting_constructor(Relocation &, const Buffer &, const Construct &, Index) const;
	void disolve_once(Buffer &, DisolveFlags) const;
	void disolve(Buffer &, DisolveFlags = DisolveFlags::eAll) const;

	// Dead code elimination
	bool strip_buffer(Buffer &, const std::vector <bool> &) const;
	bool strip_once(Buffer &) const;
	bool strip(Buffer &) const;

	// Full pass
	void apply(Buffer &) const;
	void apply(TrackedBuffer &) const;

	static Optimizer stable;
};

} // namespace jvl::thunder
