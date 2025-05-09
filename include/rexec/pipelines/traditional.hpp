#pragma once

#include <vulkan/vulkan.hpp>

#include "../../ire/mirror/solid.hpp"
#include "../../target.hpp"
#include "../collection.hpp"
#include "../contexts/fragment.hpp"
#include "../contexts/vertex.hpp"
#include "../meta/sort.hpp"
#include "../resources.hpp"

namespace jvl::rexec {

struct no_push_constant_range {};

template <resource_push_constant ... Constants>
constexpr auto push_constant_range_for_stage(const resource_collection <Constants...> &,
					     const vk::ShaderStageFlags &stage)
{
	static_assert(sizeof...(Constants) == 0, "each stage can only have at most one push constant");
	return no_push_constant_range();
}

template <resource_push_constant T>
constexpr auto push_constant_range_for_stage(const resource_collection <T> &,
					     const vk::ShaderStageFlags &stage)
{
	using value_type = T::value_type;

	return vk::PushConstantRange()
		.setOffset(T::offset)
		.setSize(sizeof(ire::solid_t <value_type>))
		.setStageFlags(stage);
}

template <resource ... Resources>
constexpr auto push_constant_range_for_resources_and_stage(const resource_collection <Resources...> &,
							   const vk::ShaderStageFlags &stage)
{
	using filtered = filter_push_constant <resource_collection <Resources...>> ::group;
	return push_constant_range_for_stage(filtered(), stage);
}

// NOTE: prototype

// TODO: vertices_t <...> for vertex buffers where each element is vertex_t

// TODO: also design a variable PendingDescriptor <...> type...
// an engine should only use PendingDescriptor <> = ValidDescriptor during rendering
// and a PendingPipeline <...> only accepts ValidDescriptors...

// template <typename ... T>
// struct PendingPipeline {
// 	~PendingPipeline() {
		// static_assert(sizeof...(T) == 0);
// 	}
// };

// TODO: vec3 with alterate precision...
template <resource_layout_in Required>
constexpr bool verify_in_from_out_set(const resource_collection <> &)
{
	return false;
}

template <resource_layout_in Required, resource_layout_out Head, resource_layout_out ... Tail>
constexpr bool verify_in_from_out_set(const resource_collection <Head, Tail...> &)
{
	if constexpr (Required::location == Head::location) {
		return std::same_as <typename Required::value_type, typename Head::value_type>;
	} else if constexpr (sizeof...(Tail)) {
		return verify_layout_in_from_out <Required> (resource_collection <Tail...> ());
	} else {
		return false;
	}
}

template <resource_layout_in ... Required, resource_layout_out ... Given>
constexpr bool verify_in_set_from_out_set(const resource_collection <Required...> &,
					  const resource_collection <Given...> &given)
{
	return (verify_in_from_out_set <Required> (given) && ...);
}

template <generic Symbolic>
constexpr vk::Format format_lookup()
{
	// TODO: format is not necessarily same as type... for now its fine
	if constexpr (std::same_as <Symbolic, ire::vec3>) {
		return vk::Format::eR32G32B32Sfloat;
	} else {
		static_assert(false,
			"unsupported vertex layout input, "
			"type is not registered in format lookup");
	}
}

// TODO: format size...
template <resource_layout_in L, resource_layout_in ... Ls>
constexpr auto vertex_input_attributes(size_t offset = 0)
{
	using value_type = typename L::value_type;

	std::array <vk::VertexInputAttributeDescription, 1 + sizeof...(Ls)> result;

	result[0] = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setFormat(format_lookup <value_type> ())
		.setOffset(offset)
		.setLocation(L::location);
	
	if constexpr (sizeof...(Ls)) {
		size_t size = sizeof(ire::solid_t <value_type>);
		auto next = vertex_input_attributes <Ls...> (offset + size);
		std::copy(next.begin(), next.end(), result.begin() + 1);
	}

	return result;
}

// TODO: use tuple_storage with multiple inheritance tails...
template <resource_layout_in ... Ls>
struct vertex_storage : std::tuple <ire::solid_t <typename Ls::value_type>...> {
	static constexpr vk::VertexInputBindingDescription binding
		= vk::VertexInputBindingDescription()
			.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(sizeof(vertex_storage));

	static constexpr auto attributes
		= vertex_input_attributes <Ls...> (0);
};

template <resource_layout_in ... Ls>
constexpr auto vertex_storage_from_inputs(const sorted_layout_inputs <Ls...> &)
{
	return vertex_storage <Ls...> ();
}

template <typename T>
concept vertex_buffer_class = true;

template <typename T>
concept push_constant_class = true;

template <vertex_buffer_class VB, push_constant_class PC>
struct TraditionalPipeline {
	using vertices_t = VB;
};

template <typename V, typename F>
struct TraditionalPipelineGroup {
	const V &vertex;
	const F &fragment;

	void compile(const vk::Device &device, const vk::RenderPass renderpass) {
		using vresources = V::rexec::collection;
		using fresources = F::rexec::collection;

		// Configure push constant ranges
		auto vpc = push_constant_range_for_resources_and_stage(vresources(), vk::ShaderStageFlagBits::eVertex);
		auto fpc = push_constant_range_for_resources_and_stage(fresources(), vk::ShaderStageFlagBits::eFragment);

		std::vector <vk::PushConstantRange> ranges;
		if constexpr (std::same_as <decltype(vpc), vk::PushConstantRange>)
			ranges.emplace_back(vpc);
		if constexpr (std::same_as <decltype(fpc), vk::PushConstantRange>)
			ranges.emplace_back(fpc);

		// Configure descriptor layout
		auto dsl_info = vk::DescriptorSetLayoutCreateInfo();

		// TODO: finish...

		auto dsl = device.createDescriptorSetLayout(dsl_info);

		// Build the pipeline layout
		auto layout_info = vk::PipelineLayoutCreateInfo()
			.setPushConstantRanges(ranges);

		auto layout = device.createPipelineLayout(layout_info);

		// Compile the actual shaders
		auto vspv = link(vertex).generate(Target::spirv_binary_via_glsl, Stage::vertex);
		auto vmod_info = vk::ShaderModuleCreateInfo().setCode(vspv.template as <BinaryResult> ());
		auto vmodule = device.createShaderModule(vmod_info);

		auto fspv = link(fragment).generate(Target::spirv_binary_via_glsl, Stage::fragment);
		auto fmod_info = vk::ShaderModuleCreateInfo().setCode(fspv.template as <BinaryResult> ());
		auto fmodule = device.createShaderModule(fmod_info);

		// Configure the rest of the rasterization pipeline
		using vertex_inputs = filter_layout_in <vresources> ::group;
		using vertex_outputs = filter_layout_out <vresources> ::group;

		using fragment_inputs = filter_layout_in <fresources> ::group;
		using fragment_outputs = filter_layout_out <fresources> ::group;

		static constexpr bool layout_compatibility = verify_in_set_from_out_set(fragment_inputs(), vertex_outputs());

		static_assert(layout_compatibility,
			"fragment shader uses layout inputs which "
			"are not specified as vertex shader outputs");

		using sorted_vertex_inputs = decltype(sort_layout_inputs(vertex_inputs()));
		using vertex_t = decltype(vertex_storage_from_inputs(sorted_vertex_inputs()));

		auto shaders = std::array <vk::PipelineShaderStageCreateInfo, 2> {
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vmodule)
				.setPName("main"),
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fmodule)
				.setPName("main"),
		};

		auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptions(vertex_t::binding)
			.setVertexAttributeDescriptions(vertex_t::attributes);

		// TODO: possibly create a index buffer abstraction
		// which depends on this topology...
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

		auto dynamic_states = std::array <vk::DynamicState, 2> {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
		};

		auto dynamic_info = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStates(dynamic_states);

		auto viewport_info = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setScissorCount(1);

		// TODO: easier way to specify this from the fragment shader...
		auto blend_attachemnt = vk::PipelineColorBlendAttachmentState()
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setBlendEnable(true);

		auto blend_info = vk::PipelineColorBlendStateCreateInfo()
			.setBlendConstants({ 0, 0, 0, 0})
			.setAttachments(blend_attachemnt)
			.setLogicOp(vk::LogicOp::eCopy)
			.setLogicOpEnable(false);

		auto pipeline_info = vk::GraphicsPipelineCreateInfo()
			.setStages(shaders)
			.setLayout(layout)
			.setPVertexInputState(&vertex_input)
			.setPInputAssemblyState(&input_assembly)
			.setPMultisampleState(&multisampling_info)
			.setPRasterizationState(&rasterization_info)
			.setPDynamicState(&dynamic_info)
			.setPViewportState(&viewport_info)
			.setPColorBlendState(&blend_info)
			.setRenderPass(renderpass);

		auto pipeline = device.createGraphicsPipelines({}, pipeline_info).value;

		// TODO: return an object with correctly available set of operations
	}

	// Verification
	static constexpr bool verify() {
		// Granural checks which fail one at a time to prevent a wall of errors ;)

		// Basic check to ensure that we can work with these instances
		if constexpr (!ire::procedure_class <V>)
			static_assert(false, "vertex program must be a procedure");
		else if constexpr (!ire::procedure_class <F>)
			static_assert(false, "fragment program must be a procedure");
		
		// Ensure that both are actually entrypoints (based on signature)
		else if constexpr (!ire::entrypoint_class <V>)
			static_assert(false, "vertex program must be an entrypoint (i.e. void())");
		else if constexpr (!ire::entrypoint_class <F>)
			static_assert(false, "fragment program must be an entrypoint (i.e. void())");
		
		// Ensure that the entrypoints are from correct REXEC contexts
		else if constexpr (!vertex_class_entrypoint <V>)
			static_assert(false, "vertex entrypoint must be from a Vertex REXEC context");
		else if constexpr (!fragment_class_entrypoint <F>)
			static_assert(false, "fragment entrypoint must be from a Fragment REXEC context");

		// TODO: resource compatbility check
		// 1. layout ins/outs...
		// 2. conflicting bindings...

		return true;
	}

	static_assert(verify());
};

} // namespace jvl::rexec