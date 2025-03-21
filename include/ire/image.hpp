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
		auto &em = Emitter::active;
		// TODO: why do we need this?
		auto type = vec <T, 4> ::type();
		auto image = em.emit_qualifier(type, binding, image_qualifiers <T> ::table[D]);
		auto value = em.emit_construct(image, -1, thunder::global);
		return cache_index_t::from(value);
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

// Implementing qualifiers
template <native T, size_t D>
struct readonly <image <T, D>> : image <T, D> {
	template <typename ... Args>
	readonly(const Args &... args) : image <T, D> (args...) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index nested = em.emit_qualifier(-1, this->binding, image_qualifiers <T> ::table[D]);
		thunder::Index qualifier = em.emit_qualifier(nested, -1, thunder::readonly);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::global);
		return cache_index_t::from(value);
	}
};

template <native T, size_t D>
struct is_image_like <readonly <image <T, D>>> : std::true_type {};

template <native T, size_t D>
struct writeonly <image <T, D>> : image <T, D> {
	template <typename ... Args>
	writeonly(const Args &... args) : image <T, D> (args...) {}

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::Index nested = em.emit_qualifier(-1, this->binding, image_qualifiers <T> ::table[D]);
		thunder::Index qualifier = em.emit_qualifier(nested, -1, thunder::writeonly);
		thunder::Index value = em.emit_construct(qualifier, -1, thunder::global);
		return cache_index_t::from(value);
	}
};

template <native T, size_t D>
struct is_image_like <writeonly <image <T, D>>> : std::true_type {};

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