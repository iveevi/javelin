#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/compile.hpp>
#include <tsg/program.hpp>
#include <tsg/filters.hpp>

#include <core/device_resource_collection.hpp>

using namespace jvl;
using namespace tsg;

// Projection matrix
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj)
		);
	}
};

// Testing
// TODO: check for duplicate push constants
auto vertex_shader(VertexIntrinsics, PushConstant <MVP> mvp, vec3 pos)
{
	vec3 p = pos;
	// cond(p.x > 0.0f);
	// 	p.y = second;
	// end();

	vec4 pp = mvp.project(p);

	return std::make_tuple(Position(pp), vec3(), vec3());
}

// TODO: deprecation warnings on unused layouts
// TODO: solid_alignment <...> and restrictions for offset based on that...
auto fragment_shader(FragmentIntrinsics, PushConstant <vec3, ire::solid_size <MVP>> color, vec3 pos, vec3)
{
	return vec4(color, 0);
}

// Device extensions
std::vector <const char *> VK_EXTENSIONS {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

int main()
{
	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	// Configure the resource collection
	core::DeviceResourceCollectionInfo info {
		.phdev = littlevk::pick_physical_device(predicate),
		.title = "Editor",
		.extent = vk::Extent2D(1920, 1080),
		.extensions = VK_EXTENSIONS,
	};
	
	auto drc = core::DeviceResourceCollection::from(info);

	// Shader compilation
	auto vs = compile_function("main", vertex_shader);
	auto fs = compile_function("main", fragment_shader);

	auto unit = Program();
	auto next = unit << vs << fs;

	using ProgramType = decltype(next);
	using FilterPushConstants = ProgramType::filter <filter_push_constants>;

	// Compile the vertex program
	thunder::opt_transform(next.ir_vertex);
	// next.ir_vertex.dump();
	auto vunit = thunder::LinkageUnit();
	vunit.add(next.ir_vertex);
	fmt::println("{}", vunit.generate_glsl());
	auto vspirv = vunit.generate_spirv(vk::ShaderStageFlagBits::eVertex);

	fmt::println("vspirv: {} bytes", sizeof(uint32_t) * vspirv.size());

	// Compile the fragment program
	thunder::opt_transform(next.ir_fragment);
	// next.ir_fragment.dump();
	auto funit = thunder::LinkageUnit();
	funit.add(next.ir_fragment);
	fmt::println("{}", funit.generate_glsl());
	auto fspirv = funit.generate_spirv(vk::ShaderStageFlagBits::eFragment);
	
	fmt::println("fspirv: {} bytes", sizeof(uint32_t) * fspirv.size());

	// Construct a render pass
	auto color_attachment = vk::AttachmentDescription()
		.setFormat(drc.swapchain.format)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	auto color_attachment_reference = vk::AttachmentReference()
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setAttachment(0);

	auto subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachments(color_attachment_reference);

	auto render_pass_info = vk::RenderPassCreateInfo()
		.setSubpasses(subpass)
		.setAttachments(color_attachment);

	auto render_pass = drc.device.createRenderPass(render_pass_info);

	// Building shader stages for Vulkan
	std::vector <vk::PipelineShaderStageCreateInfo> shaders;

	// Vertex shader stage
	auto vertex_module_info = vk::ShaderModuleCreateInfo()
		.setCode(vspirv)
		.setCodeSize(sizeof(uint32_t) * vspirv.size());

	auto vertex_module = drc.device.createShaderModule(vertex_module_info);

	auto vertex_stage_info = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(vertex_module)
		.setPName("main");

	// Fragment shader stage	
	auto fragment_module_info = vk::ShaderModuleCreateInfo()
		.setCode(fspirv)
		.setCodeSize(sizeof(uint32_t) * fspirv.size());

	auto fragment_module = drc.device.createShaderModule(fragment_module_info);

	auto fragment_stage_info = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(fragment_module)
		.setPName("main");

	// Vertex input state
	auto vertex_binding = vk::VertexInputBindingDescription()
		.setBinding(0)
		.setInputRate(vk::VertexInputRate::eVertex)
		.setStride(sizeof(float3));

	auto vertex_attributes = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(0);

	auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptions(vertex_binding)
		.setVertexAttributeDescriptions(vertex_attributes);

	// Vertex input assembly
	auto vertex_assembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList);

	// Pipeline layout
	auto layout_info = vk::PipelineLayoutCreateInfo()
		.setSetLayouts({})
		.setPushConstantRanges(FilterPushConstants::result);

	auto layout = drc.device.createPipelineLayout(layout_info);

	// Multi-sampling state
	auto multisampling_info = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	// Color blending state
	auto color_blending_info = vk::PipelineColorBlendAttachmentState()
		.setColorBlendOp(vk::BlendOp::eAdd);

	auto blending_info = vk::PipelineColorBlendStateCreateInfo()
		.setAttachments(color_blending_info);

	// Rasterization state
	auto rasterization_info = vk::PipelineRasterizationStateCreateInfo()
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f);
	
	// Dynamic viewport state
	std::array <vk::DynamicState, 2> dynamic_states {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
		
	auto dynamic_info = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	// Viewport state
	auto viewport_info = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);

	// Construct the final graphics pipeline
	std::array <vk::PipelineShaderStageCreateInfo, 2> shader_stages {
		vertex_stage_info,
		fragment_stage_info
	};

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setLayout(layout)
		.setPInputAssemblyState(&vertex_assembly)
		.setPMultisampleState(&multisampling_info)
		.setPVertexInputState(&vertex_input_info)
		.setPRasterizationState(&rasterization_info)
		.setPDynamicState(&dynamic_info)
		.setPColorBlendState(&blending_info)
		.setPViewportState(&viewport_info)
		.setRenderPass(render_pass)
		.setStages(shader_stages);

	auto pipeline = drc.device.createGraphicsPipelines(nullptr, pipeline_info).value.front();
}
