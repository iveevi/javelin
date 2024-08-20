#include <array>

#include <fmt/printf.h>

#include "ire/emitter.hpp"
#include "wrapped_types.hpp"

// Compressing IR code sequences
using block_atom_t = uint32_t;

constexpr size_t block_size = (sizeof(jvl::thunder::Atom)
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

block_t cast_to_block(const thunder::Atom &g)
{
	block_t b;
	std::memcpy(b.data(), &g, sizeof(thunder::Atom));
	return b;
}

std::tuple <size_t, wrapped::reindex <thunder::index_t>>
ir_compact_deduplicate(const thunder::Atom *const source,
		       thunder::Atom *const dst,
		       std::unordered_set <thunder::index_t> &synthesized,
		       size_t elements)
{
	wrapped::hash_table <block_t, thunder::index_t> blocks;
	std::unordered_set <thunder::index_t> original;
	wrapped::reindex <thunder::index_t> reindexer;

	// Find duplicates (binary)
	for (size_t i = 0; i < elements; i++) {
		block_t b = cast_to_block(source[i]);
		if (blocks.count(b) && !synthesized.count(i)) {
			reindexer[i] = blocks[b];
		} else {
			reindexer[i] = original.size();
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

	// Re-index all instructions as necessary
	for (size_t i = 0; i < size; i++)
		thunder::reindex_ir_operation(reindexer, dst[i]);

	return { size, reindexer };

	// TODO: structure similarity (i.e. lists)
}

} // namespace jvl::ire::detail
