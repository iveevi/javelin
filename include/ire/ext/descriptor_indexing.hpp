#pragma once

#include "../native.hpp"
#include "../sampler.hpp"
#include "../array.hpp"

namespace jvl::ire {

template <integral_native T>
auto nonuniformEXT(const native_t <T> &index)
{
	return platform_intrinsic_from_args <native_t <T>> (thunder::nonuniformEXT, index);
}

// Sampler arrays
template <native T, size_t D>
requires (D >= 1 && D <= 3)
struct array <sampler <T, D>> : public tagged {
	array(int32_t count, uint32_t binding) {
		auto &em = Emitter::active;
		auto qualifier = sampler <T, D> ::type(binding);
		auto array = em.emit_qualifier(qualifier, count, thunder::arrays);
		this->ref = em.emit_construct(array, -1, thunder::global);
	}

	// Uniform constant indexing
	sampler <T, D> operator[](uint32_t idx) const {
		auto &em = Emitter::active;
		auto loc = native_t <uint32_t> (idx);
		auto access = em.emit_array_access(this->ref.id, loc.synthesize().id);
		return cache_index_t::from(access);
	}

	// TODO: non uniform indexing
	sampler <T, D> operator[](const native_t <uint32_t> &index) const {
		auto &em = Emitter::active;
		auto loc = nonuniformEXT(index);
		auto access = em.emit_array_access(this->ref.id, loc.synthesize().id);
		return cache_index_t::from(access);
	}
};

template <native T, size_t D>
requires (D >= 1 && D <= 3)
struct unsized_array <sampler <T, D>> : public array <sampler <T, D>> {
	unsized_array(uint32_t binding) : array <sampler <T, D>> (-1, binding) {}

	// Indexing operations are inherited...
};

} // namespace jvl::ire