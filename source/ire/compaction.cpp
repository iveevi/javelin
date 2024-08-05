#include <array>

#include "ire/emitter.hpp"

// Compressing IR code sequences
using block_atom_t = uint32_t;

constexpr size_t block_size = (sizeof(jvl::ire::op::General)
				+ sizeof(block_atom_t) - 1)
				/ sizeof(block_atom_t);

using block_t = std::array <block_atom_t, block_size>;

bool operator==(const block_t &A, const block_t &B)
{
	for (size_t i = 0; i < block_size; i++) {
		if (A[i] != B[i])
			return false;
	}

	return true;
}

template <>
struct std::hash <block_t> {
	size_t operator()(const block_t &b) const {
		auto h = hash <block_atom_t> ();

		size_t x = 0;
		for (size_t i = 0; i < block_size; i++)
			x ^= h(b[i]);

		return x;
	}
};


namespace jvl::ire::detail {

block_t cast_to_block(const op::General &g)
{
	block_t b;
	std::memcpy(b.data(), &g, sizeof(op::General));
	return b;
}

size_t ir_compact_deduplicate(const op::General *const source,
		              op::General *const dst,
			      std::unordered_set <int> &main,
			      size_t elements)
{
	wrapped::hash_table<block_t, int> blocks;
	wrapped::hash_table<int, int> reindex;
	std::unordered_set<int> original;

	// Find duplicates (binary)
	// TODO: exception is if it is main
	for (size_t i = 0; i < elements; i++) {
		block_t b = cast_to_block(source[i]);
		if (blocks.count(b)) {
			reindex[i] = blocks[b];
		}
		else {
			reindex[i] = blocks.size();
			blocks[b] = blocks.size();
			original.insert(i);
		}
	}

	// Re-synthesize
	size_t size = 0;
	for (size_t i = 0; i < elements; i++) {
		if (original.count(i))
			dst[size++] = source[i];
	}

	// Fix the main instruction indices
	std::unordered_set<int> fixed_main;
	for (int i : main)
		fixed_main.insert(reindex[i]);

	main = fixed_main;

	// Re-index all instructions as necessary
	// TODO: wrap the reindexer that aborts when not found (and -1 -> -1)
	reindex[-1] = -1;

	op::reindex_vd rvd(reindex);
	for (size_t i = 0; i < size; i++) {
		std::visit(rvd, dst[i]);
	}

	return size;

	// TODO: structure similarity (i.e. lists)
}

} // namespace jvl::ire::detail
