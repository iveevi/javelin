#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/assembler.hpp>
#include <tsg/compile.hpp>
#include <tsg/filters.hpp>
#include <tsg/program.hpp>
#include <tsg/commands.hpp>

#include "aperature.hpp"
#include "vulkan_resources.hpp"
#include "transform.hpp"
#include "camera_controller.hpp"
#include "imported_asset.hpp"
#include "triangle_mesh.hpp"

using namespace jvl;
using namespace tsg;
	
MODULE(tsg-feature);

// Rasterization input assembly and basic states
template <vk::PrimitiveTopology T,
	  vk::CullModeFlagBits C = vk::CullModeFlagBits::eNone,
	  vk::PolygonMode P = vk::PolygonMode::eFill>
struct Assembly {
	static auto input_assembly() {
		return vk::PipelineInputAssemblyStateCreateInfo()
			.setPrimitiveRestartEnable(false)
			.setTopology(T);
	}
	
	static auto rasterization() {
		return vk::PipelineRasterizationStateCreateInfo()
			.setCullMode(C)
			.setPolygonMode(P);
	}
};

template <vk::CullModeFlagBits C = vk::CullModeFlagBits::eNone,
	  vk::PolygonMode P = vk::PolygonMode::eFill>
using TriangleList = Assembly <vk::PrimitiveTopology::eTriangleList, C, P>;

template <typename T>
struct is_assembly_type : std::false_type {};

template <vk::PrimitiveTopology T,
	  vk::CullModeFlagBits C,
	  vk::PolygonMode P>
struct is_assembly_type <Assembly <T, C, P>> : std::true_type {};

template <typename T>
concept assembly = is_assembly_type <T> ::value;

// Vertex layout description
template <generic ... Args>
struct VertexLayout {};

// Compile programs to intermediate representations
template <typename Args>
thunder::TrackedBuffer compile(auto program)
{
	using lifted = lift_argument <Args> ::type;

	thunder::TrackedBuffer buffer;

	ire::Emitter::active.push(buffer);
	{
		auto args = lifted();
		if constexpr (not std::is_same <Args, std::tuple <>> ::value)
			shader_args_initializer(Args(), args);
		auto results = std::apply(program, args);
		exporting(results);
	}
	ire::Emitter::active.pop();

	return buffer;
}

// Generating rasterizer pipelines
auto rasterizer(auto vertex_layout, auto vertex_transform, auto fragment_processor)
{
	return rasterizer(TriangleList <> (),
		vertex_layout,
		vertex_transform,
		fragment_processor);
}

using SPIRV = std::vector <uint32_t>;

struct RastizationSkeleton {
	std::optional <SPIRV> vertex;
	std::optional <SPIRV> fragment;
};

template <assembly A>
auto rasterizer(const A &assembly, auto vertex_layout, auto vertex_transform, auto fragment_processor)
{
	// Manually
	using Tv = decltype(function_breakdown(vertex_transform)) ::base;
	using Tf = decltype(function_breakdown(fragment_processor)) ::base;

	using Sv = signature <Tv>;
	using Sf = signature <Tf>;

	using Vin = Sv::arguments;
	using Vout = Sv::returns;

	// TODO: ensure at least a Position is returned

	using Fin = Sf::arguments;
	using Fout = Sf::returns;

	// TODO: ensure that fin and vout are compatible

	// TODO: extract the capabitlities from the IO of shaders
	// TODO: extract render pass outputs and dependencies...

	auto vbuffer = compile <Vin> (vertex_transform);
	thunder::opt_transform(vbuffer);
	vbuffer.dump();

	auto vspirv = ire::link(vbuffer).generate_spirv(vk::ShaderStageFlagBits::eVertex);
	
	auto fbuffer = compile <Fin> (fragment_processor);
	thunder::opt_transform(fbuffer);
	fbuffer.dump();
	
	auto fspirv = ire::link(vbuffer).generate_spirv(vk::ShaderStageFlagBits::eVertex);

	return RastizationSkeleton(vspirv, fspirv);
}

// Example programs
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

auto transform(PushConstant <MVP> mvp, vec3 position) -> std::tuple <Position, vec3>
{
	return { mvp.project(position), position };
}

auto coloring() -> vec3
{
	return { 1, 0.5, 0.5 };
}

// TODO: Templated vertex and index buffers

struct BindingInfo {

};

// TODO: templates...
struct Rasterizer {
	vk::RenderPass render_pass;
	vk::Pipeline pipeline;
	vk::PipelineLayout pipeline_layout;
	vk::DescriptorSetLayout descriptor_layout;
	std::map <size_t, BindingInfo> binding_points;
};

template <size_t N>
constexpr auto attachment_descriptions(const std::array <vk::Format, N> &formats)
{
	// TODO: needs to be verified with the fragment outputs

	std::array <vk::AttachmentDescription, N + 1> descriptions;
	// TODO: require depth to be one of the Rs

	descriptions[0] = vk::AttachmentDescription()
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setFormat(vk::Format::eD32Sfloat)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setSamples(vk::SampleCountFlagBits::e1);

	for (size_t i = 0; i < N; i++) {
		// TODO: color may depend, etc.
		descriptions[i + 1] = vk::AttachmentDescription()
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFormat(formats[i])
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore);
	}

	return descriptions;
}

int main()
{
	// Experimentation
	using Tv = decltype(function_breakdown(transform)) ::base;
	using Tf = decltype(function_breakdown(coloring)) ::base;

	using Sv = signature <Tv>;
	using Sf = signature <Tf>;

	using Vin = Sv::arguments;
	using Vout = Sv::returns;

	// TODO: ensure at least a Position is returned

	using Fin = Sf::arguments;
	using Fout = Sf::returns;

	// Usage
	auto layout = VertexLayout <vec3, vec3> ();
	auto assembly = TriangleList <> ();

	auto Rsk = rasterizer(assembly, layout, transform, coloring);

	// TODO: inferring render passes from the fragment shader outputs
	// subpasses are automatically created if there are explicit dependencies?

	// Configuring Vulkan devices
	auto predicate = [](const vk::PhysicalDevice &phdev) {
		return littlevk::physical_device_able(phdev, {});
	};

	auto phdev = littlevk::pick_physical_device(predicate);

	float queue_priority = 1.0f;

	auto device_queue_info = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(0)
		.setQueuePriorities(queue_priority)
		.setQueueCount(1);

	auto device_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(device_queue_info);

	auto device = phdev.createDevice(device_info);

	// Configuring the render pass
	std::array <vk::Format, 1> formats { 
		vk::Format::eR8G8B8A8Sint,
	};

	auto attachments = attachment_descriptions(formats);

	auto ref_color = vk::AttachmentReference()
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	
	auto ref_depth = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto subpass_info = vk::SubpassDescription()
		.setColorAttachments(ref_color)
		.setPDepthStencilAttachment(&ref_depth);

	auto render_pass_info = vk::RenderPassCreateInfo()
		.setSubpasses(subpass_info)
		.setAttachments(attachments);

	auto render_pass = device.createRenderPass(render_pass_info);

	// Configure the graphics pipeline
	auto vmodule_info = vk::ShaderModuleCreateInfo().setCode(Rsk.vertex.value());
	auto fmodule_info = vk::ShaderModuleCreateInfo().setCode(Rsk.fragment.value());

	auto vmodule = device.createShaderModule(vmodule_info);
	auto fmodule = device.createShaderModule(fmodule_info);

	auto stages = std::array <vk::PipelineShaderStageCreateInfo, 2> ();
	
	stages[0] = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(vmodule);
	
	stages[1] = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(vmodule);

	auto input_assembly_state = assembly.input_assembly();
	auto rasterization_state = assembly.rasterization();

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setPInputAssemblyState(&input_assembly_state)
		.setPRasterizationState(&rasterization_state)
		.setStages(stages);

	auto pipeline = device.createGraphicsPipelines({}, pipeline_info).value;
}