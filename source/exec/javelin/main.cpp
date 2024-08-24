#include <cassert>
#include <csignal>
#include <cstdlib>
#include <filesystem>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>
#include <functional>
#include <memory>
#include <random>

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
#include "littlevk/littlevk.hpp"
#include "math_types.hpp"

// Local project headers
#include "glio.hpp"
#include "device_resource_collection.hpp"
#include "host_framebuffer_collection.hpp"
#include "host_raytracer.hpp"
#include "wrapped_types.hpp"

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

// Render loop information for other contexts
struct RenderState {
	vk::CommandBuffer cmd;
	littlevk::SurfaceOperation sop;
	int32_t frame;
};

// Attachments to the global context for the main rendering loop
struct Attachment {
	using render_impl = std::function <void (const RenderState &)>;
	using resize_impl = std::function <void ()>;

	render_impl renderer;
	resize_impl resizer;
};

// Global state of the engine backend
struct GlobalContext {
	// GPU (active) resources
	DeviceResourceCollection drc;

	// Active scene
	core::Scene scene;

	// Users for each attachment
	wrapped::hash_table <std::string, void *> lut;

	// Attachments from other contexts
	// TODO: attach with the user address,
	// and then a hash table
	std::vector <Attachment> attachments;

	// Resizing
	void resize() {
		drc.combined().resize(drc.surface, drc.window, drc.swapchain);
		for (auto &attachment : attachments)
			attachment.resizer();
	}

	// Rendering
	struct {
		int32_t index = 0;
	} frame;

	void loop() {
		// Synchronization information
		auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);
	
		// Command buffers for the rendering loop
		auto command_buffers = littlevk::command_buffers(drc.device,
			drc.command_pool,
			vk::CommandBufferLevel::ePrimary, 2u);

		// Main loop
		while (!glfwWindowShouldClose(drc.window.handle)) {
			// Refresh event list
			glfwPollEvents();
		
			// Grab the next image
			littlevk::SurfaceOperation sop;
			sop = littlevk::acquire_image(drc.device, drc.swapchain.swapchain, sync[frame.index]);
			if (sop.status == littlevk::SurfaceOperation::eResize) {
				resize();
				continue;
			}
		
			// Start the render pass
			const auto &cmd = command_buffers[frame.index];

			vk::CommandBufferBeginInfo cbbi;
			cmd.begin(cbbi);
	
			littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window));

			RenderState rs;
			rs.cmd = cmd;
			rs.frame = frame.index;
			rs.sop = sop;

			for (auto &attachment : attachments)
				attachment.renderer(rs);
		
			cmd.end();
		
			// Submit command buffer while signaling the semaphore
			constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

			vk::SubmitInfo submit_info {
				sync.image_available[frame.index],
				wait_stage, cmd,
				sync.render_finished[frame.index]
			};

			drc.graphics_queue.submit(submit_info, sync.in_flight[frame.index]);

			// Present to the window
			sop = littlevk::present_image(drc.present_queue, drc.swapchain.swapchain, sync[frame.index], sop.index);
			if (sop.status == littlevk::SurfaceOperation::eResize)
				resize();
				
			// Toggle frame counter
			frame.index = 1 - frame.index;
		}
	}

	// Construction	
	GlobalContext(const GlobalContextPrelude &prelude) {
		drc.phdev = prelude.phdev;
		drc.configure_display(prelude.extent, prelude.title);
		drc.configure_device(prelude.extensions);
	}
};

// Raytracing context for the CPU
struct AttachmentRaytracingCPU {
	static constexpr const char *key = "AttachmentRaytracingCPU";

	cpu::Scene scene;

	core::Transform transform;
	core::Aperature aperature;
	core::Rayframe rayframe;

	std::mt19937 random;
	std::uniform_real_distribution <float> distribution;

	int2 extent;

	float3 radiance(const Ray &ray, int depth = 0) {
		if (depth <= 0)
			return float3(0.0f);

		// TODO: trace with min/max time for shadows...
		auto sh = scene.trace(ray);

		// TODO: get environment map lighting from scene...
		if (sh.time <= 0.0)
			return float3(0.3, 0.3, 0.4);

		assert(sh.material < scene.materials.size());
		Material &material = scene.materials[sh.material];

		if (auto em = emission(material))
			return em.value();

		auto diffuse = Diffuse(sh).load(material);
		auto scattering = diffuse.sample(-ray.direction);

		Ray next = ray;
		next.origin = sh.offset();
		next.direction = scattering.wo;

		return radiance(next, depth - 1) * scattering.brdf/scattering.pdf;
	}

	float3 radiance_samples(const Ray &ray, size_t samples) {
		float3 summed = 0;
		for (size_t i = 0; i < samples; i++)
			summed += radiance(ray, 6);

		return summed/float(samples);
	}

	Ray ray_from_pixel(float2 uv) {
		core::Ray ray;
		ray.origin = rayframe.origin;
		ray.direction = normalize(rayframe.lower_left
				+ uv.x * rayframe.horizontal
				+ (1 - uv.y) * rayframe.vertical
				- rayframe.origin);

		return ray;
	}

	// Extension data with framebuffer tiles
	struct Extra {
		int accumulated;
		int expected;
	};

	using Tile = cpu::Tile <Extra>;

	// Stored contributions in the making
	cpu::Framebuffer <float3> contributions;

	float4 raytrace(int2 ij, float2 uv, const Extra &extra) {
		Ray ray = ray_from_pixel(uv);
		float3 color = radiance(ray, 10);

		float3 &prior = contributions[ij];
		prior = prior * float(extra.accumulated);
		prior += color;
		prior = prior / float(extra.accumulated + 1);

		return float4(clamp(prior, 0, 1), 1);
	}

	// Parallelized rendering utilities
	std::unique_ptr <cpu::fixed_function_thread_pool <Tile>> thread_pool;
	std::unique_ptr <HostFramebufferCollection> fb;

	bool kernel(Tile &tile) {
		auto processor = std::bind(&AttachmentRaytracingCPU::raytrace, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);

		fb->host.process_tile(processor, tile);

		return (++tile.data.accumulated) < tile.data.expected;
	}

	// ImGui callback
	void render_imgui() {
		ImGui::Begin("CPU Raytracer");

		ImVec2 size = ImGui::GetContentRegionAvail();
		extent = { int(size.x), int(size.y) };
		aperature.aspect = size.x/size.y;
		if (!fb->empty())
			ImGui::Image(fb->descriptor, size);
	
		ImGui::End();
	}

	// Trigger the rendering
	void trigger(const Transform &transform_) {
		fb->resize(extent.x, extent.y);
		contributions.resize(extent.x, extent.y);

		int2 size { 32, 32 };
		auto tiles = fb->host.tiles <AttachmentRaytracingCPU::Extra> (size, AttachmentRaytracingCPU::Extra(0, 64));

		transform = transform_;
		rayframe = core::rayframe(aperature, transform);

		thread_pool->reset();
		thread_pool->enque(tiles);
		fb->host.clear();
		contributions.clear();
		thread_pool->begin();
	}

	AttachmentRaytracingCPU(const std::unique_ptr <GlobalContext> &global_context) {
		auto &drc = global_context->drc;

		global_context->lut[key] = this;

		scene = cpu::Scene::from(global_context->scene);
		random = std::mt19937(std::random_device()());
		distribution = std::uniform_real_distribution <float> (0, 1);

		scene.build_bvh();
		fb = std::make_unique <HostFramebufferCollection> (drc.allocator(), drc.commander());
		
		auto kernel = std::bind(&AttachmentRaytracingCPU::kernel, this, std::placeholders::_1);
		thread_pool = std::make_unique <cpu::fixed_function_thread_pool <Tile>> (8, kernel);
	}
};

// Viewport information
struct AttachmentViewport {
	static constexpr const char *key = "ViewportContext";

	// Reference to 'parent'
	GlobalContext &gctx;

	// Viewing information
	core::Aperature aperature;
	core::Transform transform;

	void handle_input() {
		static constexpr float speed = 50.0f;
		static float last_time = 0.0f;

		auto &window = gctx.drc.window;

		float delta = speed * float(glfwGetTime() - last_time);
		last_time = glfwGetTime();

		float3 velocity(0.0f);
		if (window.key_pressed(GLFW_KEY_S))
			velocity.z -= delta;
		else if (window.key_pressed(GLFW_KEY_W))
			velocity.z += delta;

		if (window.key_pressed(GLFW_KEY_D))
			velocity.x -= delta;
		else if (window.key_pressed(GLFW_KEY_A))
			velocity.x += delta;

		if (window.key_pressed(GLFW_KEY_E))
			velocity.y += delta;
		else if (window.key_pressed(GLFW_KEY_Q))
			velocity.y -= delta;

		transform.translate += transform.rotation.rotate(velocity);
	}

	// Vulkan structures
	vk::RenderPass render_pass;

	void configure_render_pass() {
		auto &drc = gctx.drc;
		render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();
	}

	// Pipeline structuring
	enum : int {
		eAlbedo,
		eNormal,
		eDepth,
		eCount,
	};

	std::array <littlevk::Pipeline, eCount> pipelines;

	void configre_normal_pipeline() {
		auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

		auto &drc = gctx.drc;

		std::string vertex_shader = ire::kernel_from_args(vertex).synthesize(profiles::opengl_450);
		std::string fragment_shader = ire::kernel_from_args(fragment).synthesize(profiles::opengl_450);

		auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
			.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

		pipelines[eNormal] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
			.with_render_pass(render_pass, 0)
			.with_vertex_layout(vertex_layout)
			.with_shader_bundle(bundle)
			.with_push_constant <MVP> (vk::ShaderStageFlagBits::eVertex, 0)
			.cull_mode(vk::CullModeFlagBits::eNone);
	}

	void configure_pipelines() {
		configre_normal_pipeline();
	}

	// Framebuffers and ImGui descriptor handle
	std::vector <littlevk::Image> targets;
	std::vector <vk::Framebuffer> framebuffers;
	std::vector <vk::DescriptorSet> imgui_descriptors;
	int2 extent;

	void configure_framebuffers() {
		auto &drc = gctx.drc;
		auto &swapchain = drc.swapchain;

		targets.clear();
		framebuffers.clear();
		imgui_descriptors.clear();

		littlevk::Image depth = drc.allocator()
			.image(drc.window.extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		targets.clear();
		for (size_t i = 0; i < drc.swapchain.images.size(); i++) {
			targets.push_back(drc.allocator()
				.image(drc.window.extent,
					vk::Format::eB8G8R8A8Unorm,
					vk::ImageUsageFlagBits::eColorAttachment
						| vk::ImageUsageFlagBits::eSampled,
					vk::ImageAspectFlagBits::eColor)
			);
		}

		littlevk::FramebufferGenerator generator {
			drc.device, render_pass,
			drc.window.extent, drc.dal
		};

		for (size_t i = 0; i < drc.swapchain.images.size(); i++)
			generator.add(targets[i].view, depth.view);

		framebuffers = generator.unpack();

		vk::Sampler sampler = littlevk::SamplerAssembler(drc.device, drc.dal);
		for (const littlevk::Image &image : targets) {
			vk::DescriptorSet dset = imgui_add_vk_texture(sampler, image.view,
					vk::ImageLayout::eShaderReadOnlyOptimal);

			imgui_descriptors.push_back(dset);
		}

		auto transition = [&](const vk::CommandBuffer &cmd) {
			for (auto &image : targets)
				image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
		};

		drc.commander().submit_and_wait(transition);
	}

	// Device goemetry instances
	// TODO: gfx::vulkan::Scene
	std::vector <vulkan::TriangleMesh> meshes;

	// Rendering
	void render(const RenderState &rs) {
		handle_input();

		const auto &rpbi = littlevk::default_rp_begin_info <2> (render_pass, framebuffers[rs.sop.index], gctx.drc.window);

		rs.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
		
		auto &ppl = pipelines[AttachmentViewport::eNormal];

		rs.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);
	
		MVP mvp;
		mvp.model = float4x4::identity();
		mvp.proj = core::perspective(aperature);
		mvp.view = transform.to_view_matrix();

		rs.cmd.pushConstants <MVP> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);

		for (const auto &vmesh : meshes) {
			rs.cmd.bindVertexBuffers(0, vmesh.vertices.buffer, { 0 });
			rs.cmd.bindIndexBuffer(vmesh.triangles.buffer, 0, vk::IndexType::eUint32);
			rs.cmd.drawIndexed(vmesh.count, 1, 0, 0, 0);
		}

		rs.cmd.endRenderPass();

		littlevk::transition(rs.cmd, targets[rs.frame], vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	// ImGui rendering
	void render_imgui() {
		bool preview_open = true;
		ImGui::Begin("Preview", &preview_open, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::Button("Render")) {
				auto rtx = (AttachmentRaytracingCPU *) gctx.lut[AttachmentRaytracingCPU::key];
				rtx->trigger(transform);
			}

			ImGui::EndMenuBar();
		}

		viewport_region.active = ImGui::IsWindowFocused();

		ImVec2 size = ImGui::GetContentRegionAvail();
		ImGui::Image(imgui_descriptors[gctx.frame.index], size);

		ImVec2 viewport = ImGui::GetItemRectSize();
		aperature.aspect = viewport.x/viewport.y;

		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();

		// TODO: should be local
		viewport_region.min = { min.x, min.y };
		viewport_region.max = { max.x, max.y };

		ImGui::End();
	}
	
	// Construction
	AttachmentViewport(const std::unique_ptr <GlobalContext> &global_context) : gctx(*global_context) {
		configure_render_pass();
		configure_framebuffers();
		configure_pipelines();
	}
};

// User interface information
struct AttachmentUI {
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

	// Rendering
	std::vector <std::function <void ()>> callbacks;

	void render(const RenderState &rs) {
		const auto &rpbi = littlevk::default_rp_begin_info <1> (render_pass, framebuffers[rs.sop.index], gctx.drc.window);
		
		rs.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		for (auto &ftn : callbacks)
			ftn();
		
		ImGui::Render();

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), rs.cmd);

		rs.cmd.endRenderPass();
	}

	// Construction
	AttachmentUI(const std::unique_ptr <GlobalContext> &global_context) : gctx(*global_context) {
		configure_render_pass();
		configure_framebuffers();
		imgui_initialize_vulkan(gctx.drc, render_pass);
	}
};

struct AttachmentInspectors {
	// Active scene
	core::Scene &scene;

	// Currently highlighted object
	int32_t selected = -1;

	// ImGui rendering
	void scene_hierarchy() {
		ImGui::Begin("Scene");

		for (int32_t i = 0; i < scene.objects.size(); i++) {
			if (ImGui::Selectable(scene.objects[i].name.c_str(), selected == i)) {
				selected = i;
			}
		}

		ImGui::End();
	};

	void object_inspector() {
		ImGui::Begin("Inspector");

		if (selected >= 0) {
			auto &obj = scene.objects[selected];

			ImGui::Text("%s", obj.name.c_str());

			if (ImGui::CollapsingHeader("Meshes")) {}
			
			if (ImGui::CollapsingHeader("Materials")) {
				ImGui::Text("Count: %lu", obj.materials.size());
			}
		}

		ImGui::End();
	};

	// Construction
	AttachmentInspectors(const std::unique_ptr <GlobalContext> &global_context)
			: scene(global_context->scene) {}
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

	auto global_context = std::make_unique <GlobalContext> (prelude);
	auto &drc = global_context->drc;

	std::filesystem::path path = argv[1];
	fmt::println("path to asset: {}", path);

	auto asset = engine::ImportedAsset::from(path).value();

	global_context->scene.add(asset);
	global_context->scene.write("main.jvlx");

	// Engine editor attachments
	auto attatchment_ui = std::make_unique <AttachmentUI> (global_context);
	auto attachment_viewport = std::make_unique <AttachmentViewport> (global_context);
	auto attachment_inspectors = std::make_unique <AttachmentInspectors> (global_context);
	auto attachment_rtx_cpu = std::make_unique <AttachmentRaytracingCPU> (global_context);

	for (auto &obj : global_context->scene.objects) {
		if (obj.geometry) {
			auto &g = obj.geometry.value();

			auto tmesh = core::TriangleMesh::from(g).value();
			auto vmesh = gfx::vulkan::TriangleMesh::from(drc.allocator(), tmesh).value();
			attachment_viewport->meshes.push_back(vmesh);
		}
	}

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	glfwSetWindowUserPointer(drc.window.handle, &attachment_viewport->transform);
	glfwSetMouseButtonCallback(drc.window.handle, button_callback);
	glfwSetCursorPosCallback(drc.window.handle, cursor_callback);

	attatchment_ui->callbacks.push_back(std::bind(&AttachmentInspectors::scene_hierarchy, attachment_inspectors.get()));
	attatchment_ui->callbacks.push_back(std::bind(&AttachmentInspectors::object_inspector, attachment_inspectors.get()));
	attatchment_ui->callbacks.push_back(std::bind(&AttachmentViewport::render_imgui, attachment_viewport.get()));
	attatchment_ui->callbacks.push_back(std::bind(&AttachmentRaytracingCPU::render_imgui, attachment_rtx_cpu.get()));

	Attachment ui_attachment;
	ui_attachment.renderer = std::bind(&AttachmentUI::render, attatchment_ui.get(), std::placeholders::_1);
	ui_attachment.resizer = std::bind(&AttachmentUI::configure_framebuffers, attatchment_ui.get());
	
	Attachment viewport_attachment;
	viewport_attachment.renderer = std::bind(&AttachmentViewport::render, attachment_viewport.get(), std::placeholders::_1);
	viewport_attachment.resizer = std::bind(&AttachmentViewport::configure_framebuffers, attachment_viewport.get());

	Attachment rtx_attachment;
	rtx_attachment.renderer = [&](const RenderState &rs) { attachment_rtx_cpu->fb->refresh(rs.cmd); };
	rtx_attachment.resizer = []() { /* Nothing */ };

	global_context->attachments.push_back(rtx_attachment);
	global_context->attachments.push_back(viewport_attachment);
	global_context->attachments.push_back(ui_attachment);

	global_context->loop();

	drc.device.waitIdle();
	drc.window.drop();
	drc.dal.drop();

	attachment_rtx_cpu->thread_pool->drop();
}
