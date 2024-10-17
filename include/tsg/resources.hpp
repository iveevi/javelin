#pragma once

#include "meta.hpp"
#include "imports.hpp"
#include "intrinsics.hpp"

namespace jvl::tsg {

template <ShaderStageFlags F, generic T, size_t Begin, size_t End>
struct PushConstantRange {};

// TODO: samplers, buffers, etc.

template <typename T>
concept resource = is_stage_meta_v <PushConstantRange, T>;

// Collection of the resources used by a program
template <resource ... Args>
struct ResourceCollection {};

template <ShaderStageFlags F, resource ... Args>
struct ProxyResourceCollection {
	using stripped = ResourceCollection <Args...>;
};

// Specialization for the layout input collection
template <ShaderStageFlags F, resource ... Inputs, typename ... Args>
struct collector <ProxyResourceCollection <F, Inputs...>, Args...> {
	using result = ProxyResourceCollection <F, Inputs...>;
};

template <ShaderStageFlags F, resource ... Inputs, typename S, typename ... Args>
struct collector <ProxyResourceCollection <F, Inputs...>, S, Args...> {
	using current = ProxyResourceCollection <F, Inputs...>;
	using result = typename collector <current, Args...> ::result;
};

template <ShaderStageFlags F, resource ... Inputs, generic T, size_t Offset, typename ... Specifiers>
struct collector <ProxyResourceCollection <F, Inputs...>, PushConstant <T, Offset>, Specifiers...> {
	static constexpr size_t I = sizeof...(Inputs);
	// TODO: need to account for the aligned version
	static constexpr size_t end = Offset + ire::solid_size <T>;
	using current = ProxyResourceCollection <F, PushConstantRange <F, T, Offset, end>, Inputs...>;
	using result = typename collector <current, Specifiers...> ::result;
};

template <ShaderStageFlags F, typename T>
struct collect_resources {
	static_assert(false);
};

template <ShaderStageFlags F, typename ... Args>
struct collect_resources <F, std::tuple <Args...>> {
	using proxy = typename collector <ProxyResourceCollection <F>, Args...> ::result;
	using type = typename proxy::stripped;
};

} // namespace jvl::tsg