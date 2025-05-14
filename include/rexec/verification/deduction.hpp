#pragma once

#include <vulkan/vulkan.hpp>

#include "../class.hpp"

namespace jvl::rexec {

////////////////////////////////////////////////////////
// Non-verbose type deduction for generated pipelines //
////////////////////////////////////////////////////////

template <ire::entrypoint_class ... Entrypoints>
auto compile_entrypoints_proxy(const Entrypoints &... entrypoints)
{
	vk::Device device;
	vk::RenderPass renderpass;
	return compile(device, renderpass, entrypoints...);
}

template <rexec_class ... Contexts>
requires (ire::entrypoint_class <decltype(Contexts::main)> && ...)
auto compile_contexts_proxy()
{
	vk::Device device;
	vk::RenderPass renderpass;
	return compile(device, renderpass, Contexts::main...);
}

#define $pipeline_entrypoints(...)	decltype(compile_entrypoints_proxy(__VA_ARGS__))
#define $pipeline(...)			decltype(compile_contexts_proxy <__VA_ARGS__> ())

} // namespace jvl::rexec