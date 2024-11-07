#include <omp.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <core/aperature.hpp>
#include <core/color.hpp>
#include <core/device_resource_collection.hpp>
#include <core/scene.hpp>
#include <core/transform.hpp>
#include <engine/camera_controller.hpp>
#include <engine/frame_render_context.hpp>
#include <engine/imgui.hpp>
#include <engine/imported_asset.hpp>
#include <gfx/cpu/scene.hpp>
#include <gfx/vulkan/scene.hpp>
#include <ire/core.hpp>

#include "common/extensions.hpp"
#include "common/default_framebuffer_set.hpp"

#include <random>
#include <vector>

using namespace jvl;
using namespace jvl::ire;

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

// View and projection information
struct ViewInfo {
	mat4 view;
	mat4 proj;

	auto layout() const {
		return uniform_layout(
			"ViewInfo",
			named_field(view),
			named_field(proj));
	}
};

auto project_vertex = [](ViewInfo &info, vec3 &translate, vec3 position)
{
	// TODO: put the buffer here...
	vec4 p = vec4(position + translate, 1);
	return info.proj * (info.view * p);
};

// Sphere geometry (represented as a collection of vertices and indices)
struct Sphere {
	std::vector <float3> vertices;
	std::vector <int3> triangles;

	Sphere(int32_t resolution, float radius) {
		// Generate sphere vertices and indices (simplified version)
		for (int32_t lat = 0; lat <= resolution; lat++) {
			float theta = lat * M_PI / resolution;
			for (int32_t lon = 0; lon < resolution; lon++) {
				float phi = lon * 2 * M_PI / resolution;

				float x = radius * sin(theta) * cos(phi);
				float y = radius * cos(theta);
				float z = radius * sin(theta) * sin(phi);

				vertices.push_back(float3(x, y, z));
			}
		}

		// Generate indices for sphere (simplified triangulation)
		for (int32_t lat = 0; lat < resolution; lat++) {
			for (int32_t lon = 0; lon < resolution; lon++) {
				int first = lat * resolution + lon;
				int second = first + resolution;

				int3 t0 = { first, second, first + 1 };
				int3 t1 = { second, second + 1, first + 1 };

				triangles.push_back(t0);
				triangles.push_back(t1);
			}
		}
	}
};

// Shader kernels for the sphere rendering
void vertex()
{
	layout_in <vec3> position(0);
	layout_out <vec3> out_position(0);

	read_only_buffer <array <vec3>> translations(0, 4);
	
	push_constant <ViewInfo> view_info;

	vec3 translate = translations[gl_InstanceIndex];
	gl_Position = project_vertex(view_info, translate, position);
	out_position = position;
}

void fragment()
{
	layout_in <vec3> position(0);
	layout_out <vec4> fragment(0);

	// Since the positions are on the sphere anyways...
	vec3 N = normalize(position);
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
}

// Function to generate random points and use them as positions for spheres
std::vector <float3> generate_random_points(int num_points, float spread)
{
	std::vector <float3> points;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution <float> dis(-spread, spread);

	for (int i = 0; i < num_points; i++)
		points.push_back(float3(dis(gen), dis(gen), dis(gen)));

	return points;
}

// Pipeline configuration for rendering spheres
littlevk::Pipeline configure_pipeline(core::DeviceResourceCollection &drc,
				      const vk::RenderPass &render_pass)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto vs_callable = procedure("main") << vertex;
	auto fs_callable = procedure("main") << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	fmt::println("{}", vertex_shader);
	fmt::println("{}", fragment_shader);

	return littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t<ViewInfo>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t<u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t<ViewInfo>))
		.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer,
				  1, vk::ShaderStageFlagBits::eVertex)
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

void jitter(std::vector <float4> &particles, float spread)
{
	std::random_device rd;
	std::vector <std::default_random_engine> engines(omp_get_max_threads());

	for (int i = 0; i < omp_get_max_threads(); ++i)
		engines[i].seed(rd());

	#pragma omp parallel for
	for (size_t i = 0; i < particles.size(); ++i) {
		int tid = omp_get_thread_num();
		std::uniform_real_distribution <float> dist(-spread, spread);

		particles[i].x += dist(engines[tid]);
		particles[i].y += dist(engines[tid]);
		particles[i].z += dist(engines[tid]);
	}
}

int main(int argc, char *argv[])
{
	// Initialize Vulkan configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}

	// Load the asset and scene
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	core::DeviceResourceCollectionInfo info{
		.phdev = littlevk::pick_physical_device(predicate),
			.title = "Point Cloud Renderer",
			.extent = vk::Extent2D(1920, 1080),
			.extensions = VK_EXTENSIONS,
	};

	auto drc = core::DeviceResourceCollection::from(info);

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

	// Generate random points for the point cloud
	int N = 1'000'000;

	// TODO: solidify... (alignment issues)
	std::vector <float3> points = generate_random_points(N, 10.0f);
	for (auto p : points)
		fmt::println("p = {} {} {}", p.x, p.y, p.z);

	std::vector <float4> aligned_points;
	for (auto p : points)
		aligned_points.push_back(float4(p, 0));

	// Prepare the sphere geometry
	int resolution = 25;
	Sphere sphere(resolution, 0.025f);

	littlevk::Buffer vb;
	littlevk::Buffer ib;
	littlevk::Buffer tb;
	std::tie(vb, ib, tb) = drc.allocator()
		.buffer(sphere.vertices,
			vk::BufferUsageFlagBits::eVertexBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(sphere.triangles,
			vk::BufferUsageFlagBits::eIndexBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(aligned_points,
			vk::BufferUsageFlagBits::eStorageBuffer
			| vk::BufferUsageFlagBits::eTransferDst);

	// Configure pipeline
	auto pipeline = configure_pipeline(drc, render_pass);

	vk::DescriptorSet set = littlevk::bind(drc.device, drc.descriptor_pool)
		.allocate_descriptor_sets(pipeline.dsl.value())
		.front();

	auto tb_info = vk::DescriptorBufferInfo()
		.setBuffer(tb.buffer)
		.setOffset(0)
		.setRange(sizeof(float4) * points.size());

	auto write = vk::WriteDescriptorSet()
		.setDstSet(set)
		.setDescriptorCount(1)
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(tb_info);

	drc.device.updateDescriptorSets(write, {}, {});

	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperture
	core::Transform camera_transform;
	core::Aperature aperature;

	// MVP structure used for push constants
	auto m_view = uniform_field(ViewInfo, view);
	auto m_proj = uniform_field(ViewInfo, proj);

	solid_t <ViewInfo> view_info;

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

		engine::FrameRenderContext context{
			drc, command_buffers[frame], sync[frame], resizer
		};

		if (!context)
			continue;

		auto &cmd = context.cmd;
		auto &sync_frame = context.sync_frame;

		// Start the command buffer
		cmd.begin(vk::CommandBufferBeginInfo());

		// Configure the rendering extent
		vk::Extent2D extent = drc.window.extent;

		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[context.sop.index])
			.with_extent(extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
			.clear_depth(1, 1)
			.begin(cmd);

		// Update the constants with the view matrix
		aperature.aspect = float(extent.width) / float(extent.height);

		auto proj = perspective(aperature);
		auto view = camera_transform.to_view_matrix();

		view_info[m_proj] = proj;
		view_info[m_view] = view;

		// Render each point as a sphere
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			pipeline.layout,
			0, set, {});
		
		cmd.pushConstants <solid_t <ViewInfo>> (pipeline.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, view_info);
		
		cmd.bindVertexBuffers(0, vb.buffer, { 0 });
		cmd.bindIndexBuffer(ib.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(3 * sphere.triangles.size(), N, 0, 0, 0);

		cmd.endRenderPass();
		cmd.end();

		// Randomly jittering particles
		static constexpr float spread = 0.05f;

		thread_local std::random_device rd;
		thread_local std::mt19937 gen(rd());
		thread_local std::uniform_real_distribution <float> dis(-spread, spread);

		jitter(aligned_points, 0.1f);

		littlevk::upload(drc.device, tb, aligned_points);
	
		// Submit command buffer while signaling the semaphore
		vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	
		vk::SubmitInfo submit_info {
			sync_frame.image_available,
			wait_stage, cmd,
			sync_frame.render_finished
		};

		drc.graphics_queue.submit(submit_info, sync_frame.in_flight);
	
		// Advance to the next frame
		frame = 1 - frame;
	}

	return 0;
}
