#include <cassert>
#include <csignal>
#include <cstdlib>
#include <filesystem>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>

// JVL headers
#include "constants.hpp"
#include "core/aperature.hpp"
#include "core/scene.hpp"
#include "core/transform.hpp"
#include "core/triangle_mesh.hpp"
#include "engine/imported_asset.hpp"
#include "engine/imported_asset.hpp"
#include "gfx/cpu/bvh.hpp"
#include "gfx/cpu/framebuffer.hpp"
#include "gfx/cpu/intersection.hpp"
#include "gfx/cpu/scene.hpp"
#include "gfx/cpu/thread_pool.hpp"
#include "gfx/vk/triangle_mesh.hpp"
#include "imgui.h"
#include "ire/callable.hpp"
#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/uniform_layout.hpp"
#include "math_types.hpp"

// Local project headers
#include "glio.hpp"
#include "device_resource_collection.hpp"
#include "host_framebuffer_collection.hpp"
#include "host_raytracer.hpp"

using namespace jvl;

// Shader sources
struct MVP {
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};

namespace {

using namespace jvl::ire;

struct mvp {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(model, view, proj);
	}
};

// TODO: with regular input/output semantics?
void vertex()
{
	layout_in <vec3> position(0);
	layout_out <vec3> color(0);

	push_constant <mvp> mvp;

	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;
	color = position;
}

void fragment()
{
	layout_in <vec3> position(0);
	layout_out <vec4> fragment(0);

	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
}

}

using namespace jvl::core;
using namespace jvl::gfx;

// Prelude information for the global context
struct GlobalContextPrelude {
	vk::PhysicalDevice phdev;
	vk::Extent2D extent;
	std::string title;
	std::vector <const char *> extensions;
};

// Global state of the engine backend
struct GlobalContext {
	// GPU (active) resources
	DeviceResourceCollection drc;

	// Active scene
	core::Scene scene;

	// If this fails, there is no point in continuing
	static GlobalContext from(const GlobalContextPrelude &prelude) {
		GlobalContext gctx;
		
		auto &drc = gctx.drc;
		drc.phdev = prelude.phdev;
		drc.configure_display(prelude.extent, prelude.title);
		drc.configure_device(prelude.extensions);

		return gctx;
	}
};

// Viewport information
struct ViewportContext {
	// Reference to 'parent'
	GlobalContext &gctx;

	// Viewing information
	core::Aperature aperature;
	core::Transform transform;

	// Vulkan structures
	vk::RenderPass render_pass;

	// Pipeline structuring
	enum : int {
		eAlbedo,
		eNormal,
		eDepth,
		eCount,
	};

	std::array <littlevk::Pipeline, eCount> pipelines;

	// Device goemetry instances
	// TODO: gfx::vulkan::Scene
	std::vector <vulkan::TriangleMesh> meshes;

	// Framebuffers and ImGui descriptor handle
	std::vector <littlevk::Image> targets;
	std::vector <vk::Framebuffer> framebuffers;
	vk::DescriptorSet imgui_descriptor;
	ivec2 extent;
};

// User interface information
struct UserInterfaceContext {
	// Reference to 'parent'
	GlobalContext &gctx;

	// Vulkan structures
	vk::RenderPass render_pass;

	void configure_render_pass() {
		auto &drc = gctx.drc;
		render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
			.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.done();
	}

	// Framebuffers
	std::vector <vk::Framebuffer> framebuffers;

	void configure_framebuffers() {
		auto &drc = gctx.drc;
		auto &swapchain = drc.swapchain;
		
		littlevk::FramebufferGenerator generator {
			drc.device, render_pass,
			drc.window.extent, drc.dal
		};

		for (size_t i = 0; i < swapchain.images.size(); i++)
			generator.add(swapchain.image_views[i]);

		framebuffers = generator.unpack();
	}

	static UserInterfaceContext from(GlobalContext &gctx) {
		UserInterfaceContext uictx { gctx };
		uictx.configure_render_pass();
		uictx.configure_framebuffers();
		return uictx;
	}
};

int main(int argc, char *argv[])
{
	littlevk::config().enable_logging = false;
	littlevk::config().abort_on_validation_error = true;

	assert(argc >= 2);

	// Device extensions
	static const std::vector <const char *> EXTENSIONS {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	GlobalContextPrelude prelude;
	prelude.phdev = littlevk::pick_physical_device(predicate);
	prelude.extent = vk::Extent2D(1920, 1080);
	prelude.title = "Javelin Engine";
	prelude.extensions = EXTENSIONS;

	auto gctx = GlobalContext::from(prelude);
	auto &drc = gctx.drc;

	std::filesystem::path path = argv[1];
	fmt::println("path to asset: {}", path);

	auto asset = engine::ImportedAsset::from(path).value();

	gctx.scene.add(asset);
	gctx.scene.write("main.jvlx");

	// TODO: non blocking bvh construction (std promise equivalent?)
	auto cpu_scene = cpu::Scene::from(gctx.scene);
	cpu_scene.build_bvh();

	// Rendering contexts
	auto uictx = UserInterfaceContext::from(gctx);

	vk::RenderPass ui_render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	vk::RenderPass viewport_render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	imgui_initialize_vulkan(drc, ui_render_pass);

	std::vector <vk::Framebuffer> ui_framebuffers;
	std::vector <vk::Framebuffer> viewport_framebuffers;
	std::vector <littlevk::Image> render_color_targets;

	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	std::vector <core::TriangleMesh> tmeshes;
	std::vector <gfx::vulkan::TriangleMesh> vmeshes;

	for (auto &g : asset.geometries) {
		auto tmesh = core::TriangleMesh::from(g).value();
		tmeshes.push_back(tmesh);

		auto vmesh = gfx::vulkan::TriangleMesh::from(drc.allocator(), tmesh).value();
		vmeshes.push_back(vmesh);
	}

	// Create a graphics pipeline
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	std::string vertex_shader = ire::kernel_from_args(vertex).synthesize(profiles::opengl_450);
	std::string fragment_shader = ire::kernel_from_args(fragment).synthesize(profiles::opengl_450);

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	littlevk::Pipeline ppl = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(viewport_render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <MVP> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);

	// Syncronization primitives
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	std::vector <vk::DescriptorSet> imgui_descriptors;

	auto export_render_targets_to_imgui = [&]() {
		imgui_descriptors.clear();

		vk::Sampler sampler = littlevk::SamplerAssembler(drc.device, drc.dal);
		for (const littlevk::Image &image : render_color_targets) {
			vk::DescriptorSet dset = imgui_add_vk_texture(sampler, image.view,
					vk::ImageLayout::eShaderReadOnlyOptimal);

			imgui_descriptors.push_back(dset);
		}
	};

	core::Aperature aperature;
	core::Transform transform;

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	MVP mvp;
	mvp.model = float4x4::identity();
	mvp.proj = core::perspective(aperature);

	auto resize = [&]() {
		drc.combined().resize(drc.surface, drc.window, drc.swapchain);

		littlevk::Image depth = drc.allocator()
			.image(drc.window.extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		render_color_targets.clear();
		for (size_t i = 0; i < drc.swapchain.images.size(); i++) {
			render_color_targets.push_back(drc.allocator()
				.image(drc.window.extent,
					vk::Format::eB8G8R8A8Unorm,
					vk::ImageUsageFlagBits::eColorAttachment
						| vk::ImageUsageFlagBits::eSampled,
					vk::ImageAspectFlagBits::eColor)
			);
		}

		littlevk::FramebufferGenerator ui_generator(drc.device, ui_render_pass, drc.window.extent, drc.dal);
		littlevk::FramebufferGenerator viewport_generator(drc.device, viewport_render_pass, drc.window.extent, drc.dal);

		for (size_t i = 0; i < drc.swapchain.images.size(); i++) {
			ui_generator.add(drc.swapchain.image_views[i]);
			viewport_generator.add(render_color_targets[i].view, depth.view);
		}

		ui_framebuffers = ui_generator.unpack();
		viewport_framebuffers = viewport_generator.unpack();

		export_render_targets_to_imgui();

		auto transition = [&](const vk::CommandBuffer &cmd) {
			for (auto &image : render_color_targets)
				image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
		};

		drc.commander().submit_and_wait(transition);

		uictx.configure_framebuffers();
	};

	resize();

	auto handle_input = [&]() {
		static float last_time = 0.0f;

		constexpr float speed = 50.0f;

		float delta = speed * float(glfwGetTime() - last_time);
		last_time = glfwGetTime();

		float3 velocity(0.0f);
		if (drc.window.key_pressed(GLFW_KEY_S))
			velocity.z -= delta;
		else if (drc.window.key_pressed(GLFW_KEY_W))
			velocity.z += delta;

		if (drc.window.key_pressed(GLFW_KEY_D))
			velocity.x -= delta;
		else if (drc.window.key_pressed(GLFW_KEY_A))
			velocity.x += delta;

		if (drc.window.key_pressed(GLFW_KEY_E))
			velocity.y += delta;
		else if (drc.window.key_pressed(GLFW_KEY_Q))
			velocity.y -= delta;

		transform.translate += transform.rotation.rotate(velocity);
	};

	glfwSetWindowUserPointer(drc.window.handle, &transform);
	glfwSetMouseButtonCallback(drc.window.handle, button_callback);
	glfwSetCursorPosCallback(drc.window.handle, cursor_callback);

	auto fb = HostFramebufferCollection(drc.allocator(), drc.commander());

	HostRaytracer rtx(cpu_scene);

	struct Extra {
		int accumulated;
		int expected;
	};

	using Tile = cpu::Tile <Extra>;

	cpu::Framebuffer <float3> contributions;

	auto raytrace = [&](int2 ij, float2 uv, const Extra &extra) -> float4 {
		Ray ray = rtx.ray_from_pixel(uv);
		float3 color = rtx.radiance(ray, 10);

		float3 &prior = contributions[ij];
		prior = prior * float(extra.accumulated);
		prior += color;
		prior = prior / float(extra.accumulated + 1);

		return float4(clamp(prior, 0, 1), 1);
	};

	auto kernel = [&fb, &raytrace](Tile &tile) -> bool {
		fb.host.process_tile(raytrace, tile);
		return (++tile.data.accumulated) < tile.data.expected;
	};

	auto thread_pool = gfx::cpu::fixed_function_thread_pool
			<Tile, decltype(kernel)>
			(std::thread::hardware_concurrency(), kernel);

	// TODO: dynamic render pass
	ImVec2 viewport;

	int32_t selected = -1;
	int32_t frame = 0;
        while (true) {
		handle_input();

		// TODO: method instead of function
		mvp.proj = core::perspective(aperature);
		mvp.view = transform.to_view_matrix();

                glfwPollEvents();
                if (glfwWindowShouldClose(drc.window.handle))
                        break;

		littlevk::SurfaceOperation op;
                op = littlevk::acquire_image(drc.device, drc.swapchain.swapchain, sync[frame]);
		if (op.status == littlevk::SurfaceOperation::eResize) {
			resize();
			continue;
		}

		// Start the render pass
		const auto &cmd = command_buffers[frame];
		cmd.begin(vk::CommandBufferBeginInfo {});

		// Set viewport and scissor
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window));

		const auto &render_rpbi = littlevk::default_rp_begin_info <2>
			(viewport_render_pass, viewport_framebuffers[op.index], drc.window);

		// Update framebuffer buffer and copy to the image
		fb.refresh(cmd);

		cmd.beginRenderPass(render_rpbi, vk::SubpassContents::eInline);
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

			cmd.pushConstants <MVP> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);

			for (const auto &vmesh : vmeshes) {
				cmd.bindVertexBuffers(0, vmesh.vertices.buffer, { 0 });
				cmd.bindIndexBuffer(vmesh.triangles.buffer, 0, vk::IndexType::eUint32);
				cmd.drawIndexed(vmesh.count, 1, 0, 0, 0);
			}
		}
		cmd.endRenderPass();

		littlevk::transition(cmd, render_color_targets[frame],
				vk::ImageLayout::ePresentSrcKHR,
				vk::ImageLayout::eShaderReadOnlyOptimal);

		const auto &rpbi = littlevk::default_rp_begin_info <1>
			(ui_render_pass, ui_framebuffers[op.index], drc.window);

		cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

			if (ImGui::Begin("Scene")) {
				for (int32_t i = 0; i < gctx.scene.objects.size(); i++) {
					if (ImGui::Selectable(gctx.scene.objects[i].name.c_str(), selected == i)) {
						selected = i;
					}
				}

				// if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				// 	selected = -1;

				ImGui::End();
			}
			
			if (ImGui::Begin("Inspector")) {
				if (selected >= 0) {
					auto &obj = gctx.scene.objects[selected];

					ImGui::Text("%s", obj.name.c_str());

					if (ImGui::CollapsingHeader("Meshes")) {

					}
					
					if (ImGui::CollapsingHeader("Materials")) {
						ImGui::Text("Count: %d", obj.materials.size());
					}
				}

				ImGui::End();
			}

			bool preview_open = true;
			if (ImGui::Begin("Preview", &preview_open, ImGuiWindowFlags_MenuBar)) {
				if (ImGui::BeginMenuBar()) {
					if (ImGui::Button("Render")) {
						fb.resize(viewport.x, viewport.y);
						contributions.resize(viewport.x, viewport.y);

						fmt::println("rendering at {} x {} resolution", fb.host.width, fb.host.height);

						int2 size { 32, 32 };
						auto tiles = fb.host.tiles <Extra> (size, Extra(0, 64));

						rtx.transform = transform;
						rtx.rayframe = core::rayframe(rtx.aperature, rtx.transform);

						thread_pool.reset();
						thread_pool.enque(tiles);
						fb.host.clear();
						contributions.clear();
						thread_pool.begin();
					}

					ImGui::EndMenuBar();
				}

				viewport_region.active = ImGui::IsWindowFocused();

				ImVec2 size = ImGui::GetContentRegionAvail();
				ImGui::Image(imgui_descriptors[frame], size);

				ImVec2 viewport = ImGui::GetItemRectSize();
				aperature.aspect = viewport.x/viewport.y;

				ImVec2 min = ImGui::GetItemRectMin();
				ImVec2 max = ImGui::GetItemRectMax();

				viewport_region.min = { min.x, min.y };
				viewport_region.max = { max.x, max.y };

				ImGui::End();
			}

			if (ImGui::Begin("CPU Raytracer")) {
				viewport = ImGui::GetContentRegionAvail();
				rtx.aperature.aspect = viewport.x/viewport.y;
				if (!fb.empty())
					ImGui::Image(fb.descriptor, viewport);
				ImGui::End();
			}

			ImGui::Render();

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		}
		cmd.endRenderPass();

		cmd.end();

		// Submit command buffer while signaling the semaphore
		constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		vk::SubmitInfo submit_info {
			sync.image_available[frame],
			wait_stage, cmd,
			sync.render_finished[frame]
		};

		drc.graphics_queue.submit(submit_info, sync.in_flight[frame]);

                op = littlevk::present_image(drc.present_queue, drc.swapchain.swapchain, sync[frame], op.index);
		if (op.status == littlevk::SurfaceOperation::eResize)
			resize();

		frame = 1 - frame;
        }

	drc.device.waitIdle();
	drc.window.drop();
	drc.dal.drop();

	thread_pool.drop();
}
