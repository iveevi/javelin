#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <argparse/argparse.hpp>

#include <ire/core.hpp>

#include "aperature.hpp"
#include "camera_controller.hpp"
#include "cpu/scene.hpp"
#include "default_framebuffer_set.hpp"
#include "device_resource_collection.hpp"
#include "extensions.hpp"
#include "imgui.hpp"
#include "imported_asset.hpp"
#include "scene.hpp"
#include "timer.hpp"
#include "transform.hpp"
#include "vulkan/scene.hpp"

using namespace jvl;
using namespace jvl::ire;

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

// Model-view-projection structure
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

// Shader kernels
void vertex()
{
	// Vertex inputs
	layout_in <vec3> position(0);
	
	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);
}

void fragment(glm::vec3 color)
{
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	fragment = vec4(color.x, color.y, color.z, 1);
}

// Constructing the graphics pipeline
littlevk::Pipeline configure_pipeline(DeviceResourceCollection &drc,
				      const vk::RenderPass &render_pass,
				      glm::vec3 color)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto vs_callable = procedure("main") << vertex;
	auto fs_callable = procedure("main") << std::make_tuple(color) << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	// TODO: automatic generation by observing used layouts
	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	return littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto controller = reinterpret_cast <CameraController *> (glfwGetWindowUserPointer(window));

	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		io.AddMouseButtonEvent(button, action);
		controller->voided = true;
		controller->dragging = false;
		return;
	}

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
	ImGuiIO &io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);

	auto controller = reinterpret_cast <CameraController *> (glfwGetWindowUserPointer(window));
	controller->handle_cursor(glm::vec2(x, y));
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("particles");
	
	program.add_argument("mesh")
		.help("input mesh");

	program.add_argument("--limit")
		.help("time limit (ms) for the execution of the program")
		.default_value(-1.0)
		.scan <'g', double> ();

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << program;
		return 1;
	}

	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = true;
		config.abort_on_validation_error = true;
	}
	
	// Load the asset and scene
	std::filesystem::path path = program.get("mesh");

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	// Configure the resource collection
	DeviceResourceCollectionInfo info {
		.phdev = littlevk::pick_physical_device(predicate),
		.title = "Editor",
		.extent = vk::Extent2D(1920, 1080),
		.extensions = VK_EXTENSIONS,
	};
	
	auto drc = DeviceResourceCollection::from(info);

	// Load the scene
	auto asset = engine::ImportedAsset::from(path).value();

	core::Scene scene;
	scene.add(asset);

	// Prepare host and device scenes
	auto host_scene = gfx::cpu::Scene::from(scene);
	auto vk_scene = gfx::vulkan::Scene::from(drc, host_scene, gfx::vulkan::SceneFlags::eDefault);

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	// Configure ImGui
	engine::configure_imgui(drc, render_pass);

	// Initial pipeline
	glm::vec3 color = glm::vec3(1, 0, 0);

	auto ppl = configure_pipeline(drc, render_pass, color);
	
	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperature
	Transform camera_transform;
	Transform model_transform;
	Aperature aperature;

	// MVP structure used for push constants
	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);
	
	solid_t <MVP> mvp;

	mvp[m_model] = model_transform.matrix();
	
	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);
	
	// Synchronization information
	auto frame = 0u;
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);
	
	// Handling camera events
	CameraController controller {
		camera_transform,
		CameraControllerSettings()
	};

	glfwSetWindowUserPointer(drc.window.handle, &controller);
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);
	
	// Handling window resizing
	auto resize = [&]() { framebuffers.resize(drc, render_pass); };

	auto render = [&](const vk::CommandBuffer &cmd, uint32_t index) {
		controller.handle_movement(drc.window);
		
		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window.extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[index])
			.with_extent(drc.window.extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 0 })
			.clear_depth(1, 1)
			.begin(cmd);
	
		// Update the constants with the view matrix
		auto &extent = drc.window.extent;
		aperature.aspect = float(extent.width)/float(extent.height);
		mvp[m_proj] = aperature.perspective();
		mvp[m_view] = camera_transform.view_matrix();
		
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);
	
		for (auto &mesh : vk_scene.meshes) {
			int mid = *mesh.material_usage.begin();

			cmd.pushConstants <solid_t <MVP>> (ppl.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);

			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
		}

		// ImGui window to configure the palette 
		{
			engine::ImGuiRenderContext context(cmd);

			ImGui::Begin("Configure Pipeline");
			
			ImGui::ColorEdit3("color", reinterpret_cast <float *> (&color));
			if (ImGui::Button("Confirm"))
				ppl = configure_pipeline(drc, render_pass, color);

			ImGui::End();
		}

		cmd.endRenderPass();
	};

	drc.swapchain_render_loop(timed(drc.window, render, program.get <double> ("limit")), resize);

	return 0;
}