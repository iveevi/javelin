#pragma once

#include "flags.hpp"
#include "program.hpp"
#include "filters.hpp"

namespace jvl::tsg {

// Pipeline assemblers
template <ShaderStageFlags F = ShaderStageFlags::eNone, specifier ... Specifiers>
struct PipelineAssembler {
	const vk::Device &device;
};

template <>
struct PipelineAssembler <ShaderStageFlags::eNone> {
	const vk::Device &device;

	template <specifier ... Specifiers>
	friend auto operator<<(const PipelineAssembler &base, const Program <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> &program) {
		return PipelineAssembler <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> (base.device, program);
	}
};

template <specifier ... Specifiers>
struct PipelineAssembler <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> {
	static constexpr auto Flag = ShaderStageFlags::eGraphicsVertexFragment;

	const vk::Device &device;

	// Intermediate information
	vk::GraphicsPipelineCreateInfo info;

	std::array <vk::PipelineShaderStageCreateInfo, 2> shaders;
	
	// Procurables
	vk::PipelineLayout layout;

	PipelineAssembler(const vk::Device &device_, const Program <Flag, Specifiers...> &program)
			: device(device_) {
		// Do as much at this stage...

		// Shader stages
		shaders = program.shaders(device);

		// Layout description
		using FilterPushConstants = filter_push_constants <Specifiers...>;

		auto layout_info = vk::PipelineLayoutCreateInfo()
			.setSetLayouts({})
			.setPushConstantRanges(FilterPushConstants::result);

		layout = device.createPipelineLayout(layout_info);

		// Transfer properties
		info.setStages(shaders);
		info.setLayout(layout);
	}
};

} // namespace jvl::tsg