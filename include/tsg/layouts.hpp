#pragma once

#include "meta.hpp"
#include "imports.hpp"

namespace jvl::tsg {

///////////////////////////
// Layout input handling //
///////////////////////////

template <ShaderStageFlags F, generic T, size_t N>
struct LayoutIn {};

template <typename T>
concept layout_input = is_stage_meta_v <LayoutIn, T>;

// Collection of the layout input dependencies of a program
template <layout_input ... Args>
struct LayoutInputCollection {};

template <ShaderStageFlags F, layout_input ... Args>
struct ProxyLayoutInputCollection {
	using stripped = LayoutInputCollection <Args...>;
};

// Specialization for the layout input collection
template <ShaderStageFlags F, layout_input ... Inputs, typename ... Args>
struct collector <ProxyLayoutInputCollection <F, Inputs...>, Args...> {
	using result = ProxyLayoutInputCollection <F, Inputs...>;
};

template <ShaderStageFlags F, layout_input ... Inputs, typename S, typename ... Args>
struct collector <ProxyLayoutInputCollection <F, Inputs...>, S, Args...> {
	using current = ProxyLayoutInputCollection <F, Inputs...>;
	using result = typename collector <current, Args...> ::result;
};

template <ShaderStageFlags F, layout_input ... Inputs, generic T, typename ... Specifiers>
struct collector <ProxyLayoutInputCollection <F, Inputs...>, ire::layout_in <T>, Specifiers...> {
	static constexpr size_t I = sizeof...(Inputs);
	using current = ProxyLayoutInputCollection <F, LayoutIn <F, T, I>, Inputs...>;
	using result = typename collector <current, Specifiers...> ::result;
};

template <ShaderStageFlags F, typename T>
struct collect_layout_inputs {
	static_assert(false);
};

template <ShaderStageFlags F, typename ... Args>
struct collect_layout_inputs <F, std::tuple <Args...>> {
	using proxy = typename collector <ProxyLayoutInputCollection <F>, Args...> ::result;
	using type = typename proxy::stripped;
};

////////////////////////////
// Layout output handling //
////////////////////////////

template <ShaderStageFlags F, generic T, size_t N>
struct LayoutOut {};

template <typename T>
concept layout_output = is_stage_meta_v <LayoutOut, T>;

// Collection of the layout outputs of a program
template <layout_output ... Args>
struct LayoutOutputCollection {};

template <ShaderStageFlags F, layout_output ... Args>
struct ProxyLayoutOutputCollection {
	using stripped = LayoutOutputCollection <Args...>;
};

// Specialization for the layout output collection
template <ShaderStageFlags F, layout_output ... Outputs, typename ... Args>
struct collector <ProxyLayoutOutputCollection <F, Outputs...>, Args...> {
	using result = ProxyLayoutOutputCollection <F, Outputs...>;
};

template <ShaderStageFlags F, layout_output ... Outputs, typename S, typename ... Args>
struct collector <ProxyLayoutOutputCollection <F, Outputs...>, S, Args...> {
	using current = ProxyLayoutOutputCollection <F, Outputs...>;
	using result = typename collector <current, Args...> ::result;
};

template <ShaderStageFlags F, layout_output ... Outputs, generic T, typename ... Specifiers>
struct collector <ProxyLayoutOutputCollection <F, Outputs...>, ire::layout_out <T>, Specifiers...> {
	static constexpr size_t I = sizeof...(Outputs);
	using current = ProxyLayoutOutputCollection <F, LayoutOut <F, T, I>, Outputs...>;
	using result = typename collector <current, Specifiers...> ::result;
};

template <ShaderStageFlags F, typename T>
struct collect_layout_outputs {
	static_assert(false);
};

template <ShaderStageFlags F, typename ... Args>
struct collect_layout_outputs <F, std::tuple <Args...>> {
	using proxy = typename collector <ProxyLayoutOutputCollection <F>, Args...> ::result;
	using type = typename proxy::stripped;
};

} // namespace jvl::tsg