#include <thunder/buffer.hpp>
#include <thunder/opt.hpp>
#include <ire/core.hpp>

#include <tsg/assembler.hpp>
#include <tsg/compile.hpp>
#include <tsg/filters.hpp>
#include <tsg/program.hpp>

#include <core/aperature.hpp>
#include <core/device_resource_collection.hpp>
#include <core/scene.hpp>
#include <core/transform.hpp>
#include <engine/camera_controller.hpp>
#include <engine/imported_asset.hpp>
#include <gfx/cpu/scene.hpp>
#include <gfx/vulkan/scene.hpp>

using namespace jvl;
using namespace tsg;
	
MODULE(tsg-feature);

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
auto vertex_shader(VertexIntrinsics, PushConstant <MVP> mvp, vec3 position)
{
	return std::make_tuple(Position(mvp.project(position)), position);
}

// TODO: deprecation warnings on unused layouts
// TODO: solid_alignment <...> and restrictions for offset based on that...
auto fragment_shader(FragmentIntrinsics, PushConstant <vec3, solid_size <MVP>> tint, vec3 position)
{
	vec3 dU = ire::dFdx(position);
	vec3 dV = ire::dFdy(position);
	vec3 N = ire::normalize(ire::cross(dU, dV));
	return vec4((0.5f + 0.5f * N) * tint, 1);
}

// Device extensions
std::vector <const char *> VK_EXTENSIONS {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto controller = reinterpret_cast <engine::CameraController *> (glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			controller->dragging = true;
		} else if (action == GLFW_RELEASE) {
			controller->dragging = false;
			controller->voided = true;
		}
	}
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	auto controller = reinterpret_cast <engine::CameraController *> (glfwGetWindowUserPointer(window));
	controller->handle_cursor(float2(x, y));
}

int main(int argc, char *argv[])
{
	JVL_ASSERT_PLAIN(argc == 2);

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
	std::array <vk::AttachmentDescription, 2> attachments;

	attachments[0] = vk::AttachmentDescription()
		.setFormat(drc.swapchain.format)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	
	attachments[1] = vk::AttachmentDescription()
		.setFormat(vk::Format::eD32Sfloat)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto color_attachment_reference = vk::AttachmentReference()
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setAttachment(0);

	auto depth_attachment_reference = vk::AttachmentReference()
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setAttachment(1);

	auto subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachments(color_attachment_reference)
		.setPDepthStencilAttachment(&depth_attachment_reference);

	auto render_pass_info = vk::RenderPassCreateInfo()
		.setSubpasses(subpass)
		.setAttachments(attachments);

	auto render_pass = drc.device.createRenderPass(render_pass_info);

	// Vertex input state
	// TODO: TSG vertex input layout?
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
		.setColorWriteMask(vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB)
		.setBlendEnable(false);

	auto blending_info = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(false)
		.setAttachments(color_blending_info);

	// Rasterization state
	auto rasterization_info = vk::PipelineRasterizationStateCreateInfo()
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setDepthBiasEnable(false)
		.setRasterizerDiscardEnable(false)
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

	// Depth stencil
	auto depth_info = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthBoundsTestEnable(false);

	// Construct the final graphics pipeline
	auto pipeline_info = assembler.info
		.setPInputAssemblyState(&vertex_assembly)
		.setPDepthStencilState(&depth_info)
		.setPMultisampleState(&multisampling_info)
		.setPVertexInputState(&vertex_input_info)
		.setPRasterizationState(&rasterization_info)
		.setPDynamicState(&dynamic_info)
		.setPColorBlendState(&blending_info)
		.setPViewportState(&viewport_info)
		.setSubpass(0)
		.setRenderPass(render_pass);

	auto pipeline = drc.device.createGraphicsPipelines(nullptr, pipeline_info).value.front();

	// Allocate a depth buffer
	littlevk::ImageCreateInfo info {
		drc.window.extent.width,
		drc.window.extent.height,
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::ImageAspectFlagBits::eDepth,
		vk::ImageType::e2D,
		vk::ImageViewType::e2D,
	};

	littlevk::Image depth = drc.allocator().image(info);

	// Allocate the framebuffers
	std::array <vk::Framebuffer, 2> framebuffers;

	auto &extent = drc.window.extent;
	auto &swp_views = drc.swapchain.image_views;

	auto framebuffer_info = vk::FramebufferCreateInfo()
		.setWidth(extent.width)
		.setHeight(extent.height)
		.setRenderPass(render_pass)
		.setLayers(1);

	for (size_t i = 0; i < framebuffers.size(); i++) {
		std::array <vk::ImageView, 2> views {
			swp_views[i],
			depth.view
		};

		framebuffer_info.setAttachments(views);
		framebuffers[i] = drc.device.createFramebuffer(framebuffer_info);
	}

	// Record the command buffer ahead of time
	// TODO: TSG command buffer alternative
	auto commands_info = vk::CommandBufferAllocateInfo()
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(framebuffers.size())
		.setCommandPool(drc.command_pool);

	auto commands = drc.device.allocateCommandBuffers(commands_info);

	// Syncronization primitives
	// TODO: TSG alternative
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Caching MVP fields
	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);

	// Configuring assets
	auto mesh = engine::ImportedAsset::from(argv[1]).value();

	auto scene = core::Scene();
	scene.add(mesh);

	auto host_scene = gfx::cpu::Scene::from(scene);
	
	auto device_scene = gfx::vulkan::Scene::from(drc,
		host_scene,
		gfx::vulkan::SceneFlags::eDefault);

	// Camera controllers
	core::Transform camera_transform;
	core::Aperature camera_aperature;
	
	engine::CameraController controller {
		camera_transform,
		engine::CameraControllerSettings()
	};

	controller.settings.speed = 1000.0f;

	glfwSetWindowUserPointer(drc.window.handle, &controller);
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);

	// Main loop
	size_t frame = 0;

	while (!drc.window.should_close()) {
		drc.window.poll();

		controller.handle_movement(drc.window);
	
		littlevk::SurfaceOperation operation;

		operation = littlevk::acquire_image(drc.device,
			drc.swapchain.handle,
			sync[frame]);

		// Rendering commands	
		auto &cmd = commands[frame];

		vk::CommandBufferBeginInfo begin_info;

		cmd.begin(begin_info);
		{
			// Configure render pass
			auto area = vk::Rect2D()
				.setOffset(vk::Offset2D(0, 0))
				.setExtent(drc.window.extent);

			std::array <vk::ClearValue, 2> clear_values;
			clear_values[0] = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
			clear_values[1] = vk::ClearDepthStencilValue(1.0f);

			// TODO: return TSG command buffer from here
			auto begin_info = vk::RenderPassBeginInfo()
				.setRenderPass(render_pass)
				.setFramebuffer(framebuffers[operation.index])
				.setClearValues(clear_values)
				.setRenderArea(area);
	
			// TODO: return TSG command buffer from here
			cmd.beginRenderPass(begin_info, vk::SubpassContents::eInline);
			{
				// Rendering area
				auto viewport = vk::Viewport()
					.setMaxDepth(1.0f)
					.setMinDepth(0.0f)
					.setX(0.0f)
					.setY(0.0f)
					.setWidth(extent.width)
					.setHeight(extent.height);

				auto scissor = vk::Rect2D()
					.setOffset(vk::Offset2D(0, 0))
					.setExtent(extent);

				cmd.setViewport(0, viewport);
				cmd.setScissor(0, scissor);

				// Configure push constants
				solid_t <MVP> mvp;

				camera_aperature.aspect = float(extent.width) / float(extent.height);

				mvp[m_model] = float4x4::identity();
				mvp[m_proj] = core::perspective(camera_aperature);
				mvp[m_view] = camera_transform.to_view_matrix();
				
				solid_t <vec3> color = float3(0.5, 0.5, 1.0);

				// Run the pipeline
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

				cmd.pushConstants <solid_t <vec3>> (assembler.layout,
					vk::ShaderStageFlagBits::eFragment,
					solid_size <MVP>, color);

				// Render all the meshes
				for (auto &mesh : device_scene.meshes) {
					int mid = *mesh.material_usage.begin();

					cmd.pushConstants <solid_t <MVP>> (assembler.layout,
						vk::ShaderStageFlagBits::eVertex,
						0, mvp);

					cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
					cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
					cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
				}
			}
			cmd.endRenderPass();
		}
		cmd.end();
	
		// Submit rendering commands
		vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		vk::SubmitInfo submit_info {
			sync[frame].image_available,
			wait_stage, cmd,
			sync[frame].render_finished
		};

		drc.graphics_queue.submit(submit_info, sync[frame].in_flight);

		// Present to the window
		operation = littlevk::present_image(drc.present_queue,
			drc.swapchain.handle,
			sync[frame],
			operation.index);

		// Next frame
		frame = 1 - frame;
	}
}