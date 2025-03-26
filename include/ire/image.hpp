#pragma once

#include "../thunder/enumerations.hpp"
#include "native.hpp"
#include "vector.hpp"
#include "memory.hpp"

namespace jvl::ire {

// Table of corresponding image QualiferKinds for each scalar type
template <native T>
struct image_qualifiers {};

template <>
struct image_qualifiers <int32_t> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::iimage_1d, // Placeholder
		thunder::iimage_1d,
		thunder::iimage_2d,
		thunder::iimage_3d,
	};
};

template <>
struct image_qualifiers <uint32_t> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::uimage_1d, // Placeholder
		thunder::uimage_1d,
		thunder::uimage_2d,
		thunder::uimage_3d,
	};
};

template <>
struct image_qualifiers <float> {
	static constexpr thunder::QualifierKind table[] = {
		thunder::image_1d, // Placeholder
		thunder::image_1d,
		thunder::image_2d,
		thunder::image_3d,
	};
};

// Image objects
template <native T, size_t D>
requires (D >= 1 && D <= 3)
struct image {
	using value_type = vec <T, 4>;
	using index_type = std::conditional_t <D == 1, native_t <int32_t>, vec <int32_t, D>>;
	using size_type = std::conditional_t <D == 1, native_t <int32_t>, vec <int32_t, D>>;

	const size_t binding;

	image(size_t binding_ = 0) : binding(binding_) {}

	cache_index_t synthesize() const {
		auto value = Emitter::active.emit_construct(qualifier(), -1, thunder::global);
		return cache_index_t::from(value);
	}

	thunder::Index qualifier() const {
		return Emitter::active.emit_qualifier(-1, binding, image_qualifiers <T> ::table[D]);
	}
};

// Concept
template <typename T>
struct is_image_like : std::false_type {};

template <native T, size_t D>
struct is_image_like <image <T, D>> : std::true_type {};

template <typename T>
concept image_like = builtin <T> && is_image_like <T> ::value;

// Accessors functions from GLSL
template <image_like Image>
auto imageSize(const Image &handle)
{
	return platform_intrinsic_from_args <typename Image::size_type>
		(thunder::glsl_image_size, handle);
}

template <image_like Image>
auto imageLoad(const Image &handle, const typename Image::index_type &loc)
{
	return platform_intrinsic_from_args <typename Image::value_type>
		(thunder::glsl_image_load, handle, loc);
}

template <image_like Image>
void imageStore(const Image &handle,
		const typename Image::index_type &idx,
		const typename Image::value_type &data)
{
	return void_platform_intrinsic_from_args
		(thunder::glsl_image_store, handle, idx, data);
}

// Implementing access qualifiers
template <image_like I>
struct readonly <I> : I {
	template <typename ... Args>
	readonly(const Args &... args) : I(args...) {}

	cache_index_t synthesize() const {
		thunder::Index value = Emitter::active.emit_construct(qualifier(), -1, thunder::global);
		return cache_index_t::from(value);
	}

	thunder::Index qualifier() const {
		return Emitter::active.emit_qualifier(I::qualifier(), -1, thunder::readonly);
	}
};

template <image_like I>
struct is_image_like <readonly <I>> : std::true_type {};

template <image_like I>
struct writeonly <I> : I {
	template <typename ... Args>
	writeonly(const Args &... args) : I(args...) {}

	cache_index_t synthesize() const {
		thunder::Index value = Emitter::active.emit_construct(qualifier(), -1, thunder::global);
		return cache_index_t::from(value);
	}

	thunder::Index qualifier() const {
		return Emitter::active.emit_qualifier(I::qualifier(), -1, thunder::writeonly);
	}
};

template <image_like I>
struct is_image_like <writeonly <I>> : std::true_type {};

// Implementing format qualifiers
enum Format {
	rgba32f,
	rgba16f,
};

constexpr thunder::QualifierKind format_to_qualifier(const Format &fmt)
{
	switch (fmt) {
	case rgba32f:
		return thunder::QualifierKind::format_rgba32f;
	case rgba16f:
		return thunder::QualifierKind::format_rgba16f;
	default:
		break;
	}

	return thunder::QualifierKind::format_rgba32f;
}

template <image_like I, Format F>
struct format : I {
	template <typename ... Args>
	format(const Args &... args) : I(args...) {}

	cache_index_t synthesize() const {
		thunder::Index value = Emitter::active.emit_construct(qualifier(), -1, thunder::global);
		return cache_index_t::from(value);
	}

	thunder::Index qualifier() const {
		return Emitter::active.emit_qualifier(I::qualifier(), -1, format_to_qualifier(F));
	}
};

template <image_like I, Format F>
struct is_image_like <format <I, F>> : std::true_type {};

// Override type generation
template <native T, size_t D>
struct type_info_generator <image <T, D>> {
	thunder::Index binding = 0;

	type_info_generator(const image <T, D> &img) : binding(img.binding) {}

	intermediate_type synthesize() const {
		auto &em = Emitter::active;
		auto qual = em.emit_qualifier(-1, binding, image_qualifiers <T> ::table[D]);
		return composite_type(qual);
	}
};

} // namespace jvl::ire
