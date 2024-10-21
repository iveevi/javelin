#pragma once

#include "flags.hpp"
#include "program.hpp"
#include "filters.hpp"

namespace jvl::tsg {

// Pipeline assemblers
template <ShaderStageFlags F = ShaderStageFlags::eNone, specifier ... Specifiers>
struct PipelineAssembler {
	const vk::Device &device;
	const vk::RenderPass &render_pass;
};

template <>
struct PipelineAssembler <ShaderStageFlags::eNone> {
	const vk::Device &device;
	const vk::RenderPass &render_pass;

	template <specifier ... Specifiers>
	friend auto operator<<(const PipelineAssembler &base, const Program <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> &program) {
		return PipelineAssembler <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> (base.device, base.render_pass, program);
	}
};

template <specifier ... Specifiers>
struct PipelineAssembler <ShaderStageFlags::eGraphicsVertexFragment, Specifiers...> {
	static constexpr auto Flag = ShaderStageFlags::eGraphicsVertexFragment;

	const vk::Device &device;
	const vk::RenderPass &render_pass;

	// Intermediate information
	vk::GraphicsPipelineCreateInfo info;

	// Individual states
	std::optional <vk::VertexInputBindingDescription> vertex_binding;
	std::vector <vk::VertexInputAttributeDescription> vertex_attributes;
	std::vector <vk::PipelineColorBlendAttachmentState> color_blendings;
	std::vector <vk::DynamicState> dynamic_states;

	std::array <vk::PipelineShaderStageCreateInfo, 2> shaders_info;

	vk::PipelineVertexInputStateCreateInfo vertex_input_info;
	vk::PipelineInputAssemblyStateCreateInfo assembly_info;
	vk::PipelineMultisampleStateCreateInfo multisampling_info;
	vk::PipelineColorBlendStateCreateInfo blending_info;
	vk::PipelineRasterizationStateCreateInfo rasterization_info;
	vk::PipelineDynamicStateCreateInfo dynamic_info;
	vk::PipelineViewportStateCreateInfo viewport_info;
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_info;

	// Procurables
	vk::PipelineLayout layout;

	// TODO: each and every state should be valid...
	// TODO: pass the render pass
	PipelineAssembler(const vk::Device &device_,
			  const vk::RenderPass &render_pass_,
			  const Program <Flag, Specifiers...> &program)
			: device(device_), render_pass(render_pass_) {
		// Do as much at this stage...

		// Shader stages
		shaders_info = program.shaders(device);

		// Layout description
		using FilterPushConstants = filter_push_constants <Specifiers...>;

		auto layout_info = vk::PipelineLayoutCreateInfo()
			.setSetLayouts({})
			.setPushConstantRanges(FilterPushConstants::result);

		layout = device.createPipelineLayout(layout_info);

		// Transfer properties
		info.setStages(shaders_info);
		info.setLayout(layout);

		// Configure the default properties
		vertex_binding = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(sizeof(float3));

		vertex_attributes.resize(1);
		vertex_attributes[0] = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(0);

		vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptions(vertex_binding.value())
			.setVertexAttributeDescriptions(vertex_attributes);

		// Vertex input assembly
		assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList);

		// Multi-sampling state
		multisampling_info = vk::PipelineMultisampleStateCreateInfo()
			.setRasterizationSamples(vk::SampleCountFlagBits::e1);

		// Color blending state
		color_blendings.resize(1);
		color_blendings[0] = vk::PipelineColorBlendAttachmentState()
			.setColorWriteMask(vk::ColorComponentFlagBits::eR
				| vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB)
			.setBlendEnable(false);

		blending_info = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setAttachments(color_blendings);

		// Rasterization state
		rasterization_info = vk::PipelineRasterizationStateCreateInfo()
			.setCullMode(vk::CullModeFlagBits::eNone)
			.setFrontFace(vk::FrontFace::eClockwise)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setDepthBiasEnable(false)
			.setRasterizerDiscardEnable(false)
			.setLineWidth(1.0f);
		
		// Dynamic viewport state
		dynamic_states.resize(2);
		dynamic_states[0] = vk::DynamicState::eViewport;
		dynamic_states[1] = vk::DynamicState::eScissor;
			
		dynamic_info = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStates(dynamic_states);

		// Viewport state
		viewport_info = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setScissorCount(1);

		// Depth stencil
		depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthCompareOp(vk::CompareOp::eLess)
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthBoundsTestEnable(false);
	}

	vk::Pipeline compile() {	
		// Transfer the defaults
		info.setPInputAssemblyState(&assembly_info)
			.setPDepthStencilState(&depth_stencil_info)
			.setPMultisampleState(&multisampling_info)
			.setPVertexInputState(&vertex_input_info)
			.setPRasterizationState(&rasterization_info)
			.setPDynamicState(&dynamic_info)
			.setPColorBlendState(&blending_info)
			.setPViewportState(&viewport_info)
			.setSubpass(0)
			.setRenderPass(render_pass);

		return device.createGraphicsPipelines(nullptr, info).value.front();
	}
};

} // namespace jvl::tsg