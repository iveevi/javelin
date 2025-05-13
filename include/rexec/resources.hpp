#pragma once

#include "../ire/concepts.hpp"
#include "../ire/mirror/solidifiable.hpp"

namespace jvl::rexec {

//////////////////////////////
// Resource implementations //
//////////////////////////////

// Layout inputs
// TODO: interpolation modes...
struct _layout_in {};

template <ire::generic T, size_t Location>
struct LayoutIn {
	static constexpr size_t location = Location;

	using check = _layout_in;
	using value_type = T;
};

template <typename T>
concept resource_layout_in = std::same_as <typename T::check, _layout_in>;

#define $layout_in(T, B, ...)	LayoutIn <T, B>

// Layout outputs
struct _layout_out {};

template <ire::generic T, size_t Location>
struct LayoutOut {
	static constexpr size_t location = Location;

	using check = _layout_out;
	using value_type = T;
};

template <typename T>
concept resource_layout_out = std::same_as <typename T::check, _layout_out>;

#define $layout_out(T, B)	LayoutOut <T, B>

// Push constants
struct _push_constant {};

template <ire::solidifiable T, size_t Offset>
struct PushConstant {
	static constexpr size_t offset = Offset;

	using check = _push_constant;
	using value_type = T;
};

template <typename T>
concept resource_push_constant = std::same_as <typename T::check, _push_constant>;

#define $push_constant(T, O)	PushConstant <T, O>

// Storage buffers
// TODO: writeonly/readonly qualifiers
struct _storage_buffer {};

template <ire::generic T, size_t Binding>
struct StorageBuffer {
	static constexpr size_t binding = Binding;

	using check = _storage_buffer;
	using value_type = T;
};

template <typename T>
concept resource_storage_buffer = std::same_as <typename T::check, _storage_buffer>;

#define $storage_buffer(T, B)	StorageBuffer <T, B>

// Storage images
struct _storage_image {};

// TODO: format enum...
template <size_t Dimension, size_t Binding>
struct StorageImage {
	using check = _storage_image;
};

template <typename T>
concept resource_storage_image = std::same_as <typename T::check, _storage_image>;

#define $storage_image(D, B)	StorageImage <D, N>

// Image samplers
struct _sampler {};

template <size_t Dimension, size_t Binding>
struct Sampler {
	using check = _sampler;
};

template <typename T>
concept resource_sampler = std::same_as <typename T::check, _sampler>;

#define $sampler(D, B)	Sampler <D, N>

// Resource concept
template <typename T>
concept resource_bindable = false
	|| resource_storage_buffer <T>
	|| resource_storage_image <T>
	|| resource_sampler <T>;

template <typename T>
concept resource = false
	|| resource_layout_in <T>
	|| resource_layout_out <T>
	|| resource_push_constant <T>
	|| resource_bindable <T>;

} // namespace jvl::rexec