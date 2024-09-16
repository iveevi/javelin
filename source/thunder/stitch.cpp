#include "thunder/opt.hpp"

namespace jvl::thunder {

MODULE(stitch);

void stitch_mapped_instructions(Buffer &result, std::vector <mapped_instruction_t> &mapped)
{
	// Reindex locals by offset
	std::vector <size_t> block_offsets;

	size_t offset = 0;
	for (size_t i = 0; i < mapped.size(); i++) {
		auto &m = mapped[i];
		auto &g = mapped[i].refs;

		// Create a map which offsets
		wrapped::reindex <index_t> reindex;
		for (size_t i = 0; i < m.pointer; i++)
			reindex[i] = i + offset;

		// Preserve global refs
		struct ref_state_t : ref_index_t {
			index_t v0 = -1;
			index_t v1 = -1;
		};

		auto ref_state_from = [&](const ref_index_t &ri) {
			ref_state_t state(ri);
			auto &&addrs = m.atoms[ri.index].addresses();
			state.v0 = addrs.a0;
			state.v1 = addrs.a1;
			return state;
		};

		auto ref_state_restore = [&](const ref_state_t &state) {
			auto &&addrs = m.atoms[state.index].addresses();
			if (state.v0 != -1 && (state.mask & 0b01) == 0b01)
				addrs.a0 = state.v0;
			if (state.v1 != -1 && (state.mask & 0b10) == 0b10)
				addrs.a1 = state.v1;
		};

		std::vector <ref_state_t> store;
		for (auto &r : g)
			store.emplace_back(ref_state_from(r));

		// Reindex each atom
		for (size_t i = 0; i < m.pointer; i++)
			m.atoms[i].reindex(reindex);

		// Restore global refs
		for (size_t i = 0; i < store.size(); i++)
			ref_state_restore(store[i]);

		offset += m.pointer;

		block_offsets.push_back(offset - 1);
	}

	// Reindex the globals; doing it after because
	// some instructions (e.g. branches/loops) have
	// forward looking addresses
	for (size_t i = 0; i < mapped.size(); i++) {
		auto &m = mapped[i];
		auto &g = mapped[i].refs;

		for (auto &r : g) {
			auto &&addrs = m[r.index].addresses();
			if (addrs.a0 != -1 && (r.mask & 0b01) == 0b01)
				addrs.a0 = block_offsets[addrs.a0];
			if (addrs.a1 != -1 && (r.mask & 0b10) == 0b10)
				addrs.a1 = block_offsets[addrs.a1];
		}
	}

	// Stitch the independent scratches
	for (auto &m : mapped) {
		for (size_t i = 0; i < m.pointer; i++) {
			// Sanity check to ensure addresses point backwards
			auto &&addrs = m[i].addresses();
			JVL_ASSERT(addrs.a0 < (index_t) result.pointer
				&& addrs.a1 < (index_t) result.pointer,
				"instruction addresses are out of bounds: {}", m[i]);

			result.emit(m[i]);
		}
	}
}

} // namespace jvl::thunder
