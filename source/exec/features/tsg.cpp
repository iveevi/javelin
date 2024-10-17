#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/compile.hpp>
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

// Shader pipeline
struct Pipline {
};

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
	vk::AttachmentDescription color_attachment;
	color_attachment.setFormat(drc.swapchain.format);
	color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	color_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
	color_attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference color_attachment_reference;
	color_attachment_reference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	color_attachment_reference.setAttachment(0);

	vk::SubpassDescription subpass;
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.setColorAttachments(color_attachment_reference);

	vk::RenderPassCreateInfo render_pass_info;
	render_pass_info.setSubpasses(subpass);
	render_pass_info.setAttachments(color_attachment);

	auto render_pass = drc.device.createRenderPass(render_pass_info);

	// Building shader stages for Vulkan
	std::vector <vk::PipelineShaderStageCreateInfo> shaders;

	// Vertex shader stage
	vk::ShaderModuleCreateInfo vertex_module_info;
	vertex_module_info.setCode(vspirv);
	vertex_module_info.setCodeSize(sizeof(uint32_t) * vspirv.size());

	auto vertex_module = drc.device.createShaderModule(vertex_module_info);

	vk::PipelineShaderStageCreateInfo vertex_stage_info;
	vertex_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
	vertex_stage_info.setModule(vertex_module);
	vertex_stage_info.setPName("main");

	// Fragment shader stage	
	vk::ShaderModuleCreateInfo fragment_module_info;
	fragment_module_info.setCode(fspirv);
	fragment_module_info.setCodeSize(sizeof(uint32_t) * fspirv.size());

	auto fragment_module = drc.device.createShaderModule(fragment_module_info);

	vk::PipelineShaderStageCreateInfo fragment_stage_info;
	fragment_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
	fragment_stage_info.setModule(fragment_module);
	fragment_stage_info.setPName("main");

	// Vertex input state
	vk::VertexInputBindingDescription vertex_binding;
	vertex_binding.setBinding(0);
	vertex_binding.setInputRate(vk::VertexInputRate::eVertex);
	vertex_binding.setStride(sizeof(float3));

	vk::VertexInputAttributeDescription vertex_attributes;
	vertex_attributes.setBinding(0);
	vertex_attributes.setLocation(0);
	vertex_attributes.setFormat(vk::Format::eR32G32B32Sfloat);
	vertex_attributes.setOffset(0);

	vk::PipelineVertexInputStateCreateInfo vertex_input_info;
	vertex_input_info.setVertexBindingDescriptions(vertex_binding);
	vertex_input_info.setVertexAttributeDescriptions(vertex_attributes);

	// Vertex input assembly
	vk::PipelineInputAssemblyStateCreateInfo vertex_assembly;
	vertex_assembly.setTopology(vk::PrimitiveTopology::eTriangleList);

	// Pipeline layout
	vk::PushConstantRange vertex_range;
	vertex_range.setOffset(0);
	vertex_range.setSize(ire::solid_size <MVP>);
	vertex_range.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	vk::PushConstantRange fragment_range;
	fragment_range.setOffset(ire::solid_size <MVP>);
	fragment_range.setSize(ire::solid_size <vec3>);
	fragment_range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	std::array <vk::PushConstantRange, 2> push_constants {
		vertex_range,
		fragment_range
	};

	vk::PipelineLayoutCreateInfo layout_info;
	layout_info.setSetLayouts({});
	layout_info.setPushConstantRanges(push_constants);

	auto layout = drc.device.createPipelineLayout(layout_info);

	// Multi-sampling state
	vk::PipelineMultisampleStateCreateInfo multisampling_info;
	multisampling_info.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	// Color blending state
	vk::PipelineColorBlendAttachmentState color_blending_info;
	color_blending_info.setColorBlendOp(vk::BlendOp::eAdd);

	vk::PipelineColorBlendStateCreateInfo blending_info;
	blending_info.setAttachments(color_blending_info);

	// Rasterization state
	vk::PipelineRasterizationStateCreateInfo rasterization_info;
	rasterization_info.setCullMode(vk::CullModeFlagBits::eNone);
	rasterization_info.setPolygonMode(vk::PolygonMode::eFill);
	rasterization_info.setLineWidth(1.0f);
	
	// Dynamic viewport state
	std::array <vk::DynamicState, 2> dynamic_states {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
		
	vk::PipelineDynamicStateCreateInfo dynamic_info;
	dynamic_info.setDynamicStates(dynamic_states);

	// Viewport state
	vk::PipelineViewportStateCreateInfo viewport_info;
	viewport_info.setViewportCount(1);
	viewport_info.setScissorCount(1);

	// Construct the final graphics pipeline
	std::array <vk::PipelineShaderStageCreateInfo, 2> shader_stages {
		vertex_stage_info,
		fragment_stage_info
	};

	vk::GraphicsPipelineCreateInfo pipeline_info;
	pipeline_info.setLayout(layout);
	pipeline_info.setPInputAssemblyState(&vertex_assembly);
	pipeline_info.setPMultisampleState(&multisampling_info);
	pipeline_info.setPVertexInputState(&vertex_input_info);
	pipeline_info.setPRasterizationState(&rasterization_info);
	pipeline_info.setPDynamicState(&dynamic_info);
	pipeline_info.setPColorBlendState(&blending_info);
	pipeline_info.setPViewportState(&viewport_info);
	pipeline_info.setRenderPass(render_pass);
	pipeline_info.setStages(shader_stages);

	auto pipeline = drc.device.createGraphicsPipelines(nullptr, pipeline_info).value.front();
}
