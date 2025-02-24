#pragma once

#include "memory.hpp"
#include "qualified_wrapper.hpp"

namespace jvl::ire {

/////////////////////////////////////////////
// Uniform, storage buffers and references //
/////////////////////////////////////////////

template <generic T>
using uniform = qualified_wrapper <T, thunder::uniform_buffer, nullptr>;

template <generic T>
using buffer = qualified_wrapper <T, thunder::storage_buffer, nullptr>;

template <generic T>
struct buffer_reference : public qualified_wrapper <T, thunder::buffer_reference, nullptr> {
	static cache_index_t construct(const native_t <uint64_t> &address) {
		auto &em = Emitter::active;
		auto list = em.emit_list(address.synthesize().id);
		return cache_index_t::from(list);
	}

	buffer_reference(const native_t <uint64_t> &address)
			: qualified_wrapper <T, thunder::buffer_reference, nullptr>
				(construct(address), thunder::normal, type_counter <T> ()) {}
};

// Concept for buffer types
template <typename T>
struct is_buffer_like : std::false_type {};

template <generic T>
struct is_buffer_like <buffer <T>> : std::true_type {};

template <generic T>
struct is_buffer_like <uniform <T>> : std::true_type {};

template <generic T>
struct is_buffer_like <buffer_reference <T>> : std::true_type {};

template <typename T>
concept buffer_like = generic <T> && is_buffer_like <T> ::value;

// Extended qualifiers for buffers
template <buffer_like Buffer, thunder::QualifierKind kind>
inline thunder::Index qualifier_extension(thunder::Index idx)
{
	static auto &ext = Buffer::extension;

	auto &em = Emitter::active;
	if (ext)
		idx = ext(idx);

	return em.emit_qualifier(idx, -1, kind);
}

template <buffer_like Buffer>
struct writeonly <Buffer> : qualified_wrapper <
	typename Buffer::base,
	Buffer::qualifier,
	qualifier_extension <Buffer, thunder::writeonly>
> {};

template <buffer_like Buffer>
struct readonly <Buffer> : qualified_wrapper <
	typename Buffer::base,
	Buffer::qualifier,
	qualifier_extension <Buffer, thunder::readonly>
> {};

template <buffer_like Buffer>
struct scalar <Buffer> : qualified_wrapper <
	typename Buffer::base,
	Buffer::qualifier,
	qualifier_extension <Buffer, thunder::scalar>
> {};

// Enable chained qualifiers
template <buffer_like T>
struct is_buffer_like <readonly <T>> : std::true_type {};

template <buffer_like T>
struct is_buffer_like <writeonly <T>> : std::true_type {};

template <buffer_like T>
struct is_buffer_like <scalar <T>> : std::true_type {};

} // namespace jvl::ire