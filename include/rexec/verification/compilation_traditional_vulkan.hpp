#pragma once

#include <vulkan/vulkan.hpp>

#include "../../ire/procedure/ordinary.hpp"
#include "../../ire/linking.hpp"
#include "../collection.hpp"
#include "../pipeline/specifications.hpp"
#include "compilation_results.hpp"

namespace jvl::rexec {

template <typename VertexElement, typename VertexConstants, typename FragmentConstants>
auto compile_traditional_vulkan(const vk::Device &device,
				const vk::RenderPass &renderpass,
				const VerifyTraditionalResult <VertexElement,
							       VertexConstants,
							       FragmentConstants> &result,
				const ire::Procedure <void> &vshader,
				const ire::Procedure <void> &fshader)
				-> TraditionalPipeline <VertexElement,
							VertexConstants,
							FragmentConstants,
							resource_collection <>>
{
	static_assert(std::same_as <VertexElement, void>);
	
	// TODO: vertex attributes and binding from pad_tuple

	////////////////////////////////////////
	// Building the descriptor set layout //
	////////////////////////////////////////

	auto dsl_info = vk::DescriptorSetLayoutCreateInfo();

	auto dsl = device.createDescriptorSetLayout(dsl_info);
	
	//////////////////////////////////
	// Building the pipeline layout //
	//////////////////////////////////

	// Handling push constants
	std::vector <vk::PushConstantRange> ranges;

	if constexpr (!std::same_as <VertexConstants, void>) {
		auto vrange = vk::PushConstantRange()
			.setOffset(VertexConstants::numerical)
			.setSize(sizeof(typename VertexConstants::value_type))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex);
		
		ranges.emplace_back(vrange);
	}
	
	if constexpr (!std::same_as <VertexConstants, void>) {
		auto frange = vk::PushConstantRange()
			.setOffset(FragmentConstants::numerical)
			.setSize(sizeof(typename FragmentConstants::value_type))
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		
		ranges.emplace_back(frange);
	}
		
	auto layout_info = vk::PipelineLayoutCreateInfo()
		.setPushConstantRanges(ranges);

	auto layout = device.createPipelineLayout(layout_info);
	
	//////////////////////////////////////////
	// Generating and compiling the shaders //
	//////////////////////////////////////////
		
	auto vspv = link(vshader).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eVertex);
	auto vmod_info = vk::ShaderModuleCreateInfo().setCode(vspv);
	auto vmodule = device.createShaderModule(vmod_info);

	auto fspv = link(fshader).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eFragment);
	auto fmod_info = vk::ShaderModuleCreateInfo().setCode(fspv);
	auto fmodule = device.createShaderModule(fmod_info);
		
	auto shader_stages_info = std::array <vk::PipelineShaderStageCreateInfo, 2> {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vmodule)
			.setPName("main"),
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fmodule)
			.setPName("main"),
	};
	
	/////////////////////////////////
	// Building the final pipeline //
	/////////////////////////////////

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
		.setPVertexAttributeDescriptions(nullptr)
		.setPVertexBindingDescriptions(nullptr);
	
	// TODO: fill if not void
	// 	.setVertexBindingDescriptions(vertex_t::binding)
	// 	.setVertexAttributeDescriptions(vertex_t::attributes);

	// TODO: index buffer construction...
	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(false);

	// TODO: options in the fragment REXEC context...
	auto multisampling_info = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	// TODO: options in the vertex REXEC context
	auto rasterization_info = vk::PipelineRasterizationStateCreateInfo()
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setLineWidth(1);

	// TODO: abstract the render pass...
	// TODO: rexec for render pass!!
	auto dynamic_states = std::array <vk::DynamicState, 2> {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	auto dynamic_info = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	auto viewport_info = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);

	// TODO: options in framebuffer rexec
	// TODO: options from render pass rexec?
	auto blend_attachemnt = vk::PipelineColorBlendAttachmentState()
		.setBlendEnable(false)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA);

	auto blend_info = vk::PipelineColorBlendStateCreateInfo()
		.setBlendConstants({ 0, 0, 0, 0 })
		.setAttachments(blend_attachemnt)
		.setLogicOp(vk::LogicOp::eCopy)
		.setLogicOpEnable(false);

	// TODO: options in the vertex rexec
	auto depth_info = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthTestEnable(false)
		.setDepthWriteEnable(false)
		.setDepthBoundsTestEnable(false);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setStages(shader_stages_info)
		.setLayout(layout)
		.setPVertexInputState(&vertex_input)
		.setPInputAssemblyState(&input_assembly)
		.setPMultisampleState(&multisampling_info)
		.setPRasterizationState(&rasterization_info)
		.setPDynamicState(&dynamic_info)
		.setPViewportState(&viewport_info)
		.setPColorBlendState(&blend_info)
		.setPDepthStencilState(&depth_info)
		.setRenderPass(renderpass);

	auto compiled = device.createGraphicsPipelines({}, pipeline_info);
	auto handle = compiled.value.front();

	return { handle, layout, dsl };
}

} // namespace jvl::rexec