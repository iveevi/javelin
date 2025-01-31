#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <ire/core.hpp>

#include "aperature.hpp"
#include "camera_controller.hpp"
#include "color.hpp"
#include "cpu/scene.hpp"
#include "default_framebuffer_set.hpp"
#include "device_resource_collection.hpp"
#include "extensions.hpp"
#include "imgui.hpp"
#include "imported_asset.hpp"
#include "scene.hpp"
#include "transform.hpp"
#include "vulkan/scene.hpp"

using namespace jvl;
using namespace jvl::ire;

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

// Palette generation from SL values
array <vec3> hsl_palette(float saturation, float lightness, int N)
{
	std::vector <vec3> palette(N);

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		float3 hsl = float3(i * step, saturation, lightness);
		float3 rgb = core::hsl_to_rgb(hsl);
		palette[i] = vec3(rgb.x, rgb.y, rgb.z);
	}

	return palette;
}

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

	// Object triangle index
	layout_out <uint32_t, flat> out_id(0);

	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);

	// Transfering triangle index
	out_id = u32(gl_VertexIndex / 3);
}

void fragment(float saturation, float lightness, int splits)
{
	// Triangle index
	layout_in <uint32_t, flat> id(0);

	// Resulting fragment color
	layout_out <vec4> fragment(0);

	auto palette = hsl_palette(saturation, lightness, splits);

	fragment = vec4(palette[id % palette.length], 1);
}

// Constructing the graphics pipeline
littlevk::Pipeline configure_pipeline(core::DeviceResourceCollection &drc,
				      gfx::vulkan::VertexFlags flags,
				      const vk::RenderPass &render_pass,
				      float saturation,
				      float lightness,
				      int splits)
{
	auto vs_callable = procedure("main")
		<< vertex;

	auto fs_callable = procedure("main")
		<< std::make_tuple(saturation, lightness, splits)
		<< fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	fmt::println("{}", vertex_shader);
	fmt::println("{}", fragment_shader);

	// TODO: automatic generation by observing used layouts
	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	auto [binding, attributes] = gfx::vulkan::binding_and_attributes(flags);

	return littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_binding(binding)
		.with_vertex_attributes(attributes)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto controller = reinterpret_cast <engine::CameraController *> (glfwGetWindowUserPointer(window));

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

	auto controller = reinterpret_cast <engine::CameraController *> (glfwGetWindowUserPointer(window));
	controller->handle_cursor(float2(x, y));
}

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}

	// Load the asset and scene
	std::filesystem::path path = argv[1];

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

	// Load the scene
	auto asset = engine::ImportedAsset::from(path).value();
	auto scene = core::Scene();
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
	float saturation = 0.5f;
	float lightness = 1.0f;
	int splits = 16;

	auto &sampled = vk_scene.meshes[0];
	auto ppl = configure_pipeline(drc,
		sampled.flags,
		render_pass,
		saturation,
		lightness,
		splits);

	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperature
	core::Transform camera_transform;
	core::Transform model_transform;
	core::Aperature aperature;

	// MVP structure used for push constants
	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);

	solid_t <MVP> mvp;

	mvp[m_model] = model_transform.to_mat4();

	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	// Synchronization information
	auto frame = 0u;
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Handling camera events
	engine::CameraController controller {
		camera_transform,
		engine::CameraControllerSettings()
	};

	glfwSetWindowUserPointer(drc.window.handle, &controller);
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);

	// Handling window resizing
	auto resizer = [&]() { framebuffers.resize(drc, render_pass); };

	// Main loop
	auto &window = drc.window;
	while (!window.should_close()) {
		window.poll();

		controller.handle_movement(drc.window);

		const auto &cmd = command_buffers[frame];
		const auto &sync_frame = sync[frame];
		
		auto sop = littlevk::acquire_image(drc.device, drc.swapchain.handle, sync_frame);
		if (sop.status == littlevk::SurfaceOperation::eResize) {
			resizer();
			continue;
		}

		// Start the command buffer
		cmd.begin(vk::CommandBufferBeginInfo());

		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window.extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[sop.index])
			.with_extent(drc.window.extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
			.clear_depth(1, 1)
			.begin(cmd);

		// Update the constants with the view matrix
		auto &extent = drc.window.extent;
		aperature.aspect = float(extent.width)/float(extent.height);
		mvp[m_proj] = core::perspective(aperature);
		mvp[m_view] = camera_transform.to_view_matrix();

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

			// TODO: preview the colors in a grid
			ImGui::Begin("Configure Palette");

			ImGui::SliderFloat("saturation", &saturation, 0, 1);
			ImGui::SliderFloat("lightness", &lightness, 0, 1);
			ImGui::SliderInt("splits", &splits, 4, 64);
			if (ImGui::Button("Confirm")) {
				ppl = configure_pipeline(drc,
					sampled.flags,
					render_pass,
					saturation,
					lightness,
					splits);
			}

			ImGui::End();
		}

		cmd.endRenderPass();
		cmd.end();

		// Submit command buffer while signaling the semaphore
		vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		
		vk::SubmitInfo submit_info {
			sync_frame.image_available,
			wait_stage, cmd,
			sync_frame.render_finished
		};

		drc.graphics_queue.submit(submit_info, sync_frame.in_flight);

		sop = littlevk::present_image(drc.present_queue, drc.swapchain.handle, sync_frame, sop.index);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			resizer();

		// Advance to the next frame
		frame = 1 - frame;
	}
}
