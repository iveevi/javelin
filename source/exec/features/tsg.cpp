#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/assembler.hpp>
#include <tsg/compile.hpp>
#include <tsg/filters.hpp>
#include <tsg/program.hpp>

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
auto vertex_shader(VertexIntrinsics, PushConstant <MVP> mvp, vec3 pos)
{
	vec3 p = pos;
	// cond(p.x > 0.0f);
	// 	p.y = second;
	// end();

	vec4 pp = mvp.project(p);

	return std::make_tuple(Position(pp));
}

// TODO: deprecation warnings on unused layouts
// TODO: solid_alignment <...> and restrictions for offset based on that...
auto fragment_shader(FragmentIntrinsics, PushConstant <vec3, ire::solid_size <MVP>> color)
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
	core::DeviceResourceCollectionInfo drc_info {
		.phdev = littlevk::pick_physical_device(predicate),
		.title = "Editor",
		.extent = vk::Extent2D(1920, 1080),
		.extensions = VK_EXTENSIONS,
	};
	
	auto drc = core::DeviceResourceCollection::from(drc_info);

	// Shader compilation
	auto vs = compile_function("main", vertex_shader);
	auto fs = compile_function("main", fragment_shader);

	auto program = Program() << vs << fs;
	auto assembler = PipelineAssembler(drc.device) << program;

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
	auto pipeline_info = assembler.info
		.setPInputAssemblyState(&vertex_assembly)
		.setPMultisampleState(&multisampling_info)
		.setPVertexInputState(&vertex_input_info)
		.setPRasterizationState(&rasterization_info)
		.setPDynamicState(&dynamic_info)
		.setPColorBlendState(&blending_info)
		.setPViewportState(&viewport_info)
		.setRenderPass(render_pass);

	auto pipeline = drc.device.createGraphicsPipelines(nullptr, pipeline_info).value.front();
}
