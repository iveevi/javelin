#pragma once

#include "artifact.hpp"
#include "flags.hpp"
#include "resources.hpp"

namespace jvl::tsg {

// Conversion to Vulkan stage flags
constexpr vk::ShaderStageFlagBits to_vulkan(const ShaderStageFlags &flags)
{
	switch (flags) {
	case ShaderStageFlags::eVertex:
		return vk::ShaderStageFlagBits::eVertex;
	case ShaderStageFlags::eFragment:
		return vk::ShaderStageFlagBits::eFragment;
	default:
		break;
	}

	return vk::ShaderStageFlagBits::eAll;
}

//////////////////////////////
// Filtering push constants //
//////////////////////////////

template <specifier ... Specifiers>
struct filter_push_constants_impl {
	template <size_t N>
	static consteval auto result(const std::array <vk::PushConstantRange, N> &in) {
		return in;
	}
};

template <specifier S, specifier ... Specifiers>
struct filter_push_constants_impl <S, Specifiers...> {
	using next = filter_push_constants_impl <Specifiers...>;

	template <size_t N>
	static consteval auto result(const std::array <vk::PushConstantRange, N> &in) {
		if constexpr (sizeof...(Specifiers))
			return next::result(in);
		else
			return in;
	}
};

template <ShaderStageFlags F, generic T, size_t Begin, size_t End, specifier ... Specifiers>
struct filter_push_constants_impl <PushConstantRange <F, T, Begin, End>, Specifiers...> {
	using next = filter_push_constants_impl <Specifiers...>;

	template <size_t N>
	static consteval auto result(const std::array <vk::PushConstantRange, N> &in) {
		std::array <vk::PushConstantRange, N + 1> out;

		out[N] = vk::PushConstantRange()
			.setStageFlags(to_vulkan(F))
			.setOffset(Begin)
			.setSize(End - Begin);

		for (size_t i = 0; i < N; i++)
			out[i] = in[i];

		if constexpr (sizeof...(Specifiers))
			return next::result(out);
		else
			return out;
	}
};

template <specifier ... Specifiers>
struct filter_push_constants {
	static constexpr auto in = std::array <vk::PushConstantRange, 0> ();
	static constexpr auto result = filter_push_constants_impl <Specifiers...> ::result(in);
};

/////////////////////////////////////////////
// Filtering stage-specific push constants //
/////////////////////////////////////////////

struct NullPushConstants {};

template <ShaderStageFlags Stage, specifier ... Specifiers>
struct filter_stage_push_constants_impl {
	using result = NullPushConstants();
};

template <ShaderStageFlags Stage, specifier S, specifier ... Specifiers>
struct filter_stage_push_constants_impl <Stage, S, Specifiers...> {
	using result = filter_stage_push_constants_impl <Stage, Specifiers...> ::result;
};

template <ShaderStageFlags Stage, generic T, size_t Begin, size_t End, specifier ... Specifiers>
struct filter_stage_push_constants_impl <Stage, PushConstantRange <Stage, T, Begin, End>, Specifiers...> {
	using result = PushConstantRange <Stage, T, Begin, End>;
};

template <ShaderStageFlags Stage, specifier ... Specifiers>
struct filter_stage_push_constants {
	using result = filter_stage_push_constants_impl <Stage, Specifiers...> ::result;
};

} // namespace jvl::tsg