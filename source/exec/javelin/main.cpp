#include <cassert>
#include <csignal>
#include <cstdlib>
#include <filesystem>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>

#include <littlevk/littlevk.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <vulkan/vulkan_structs.hpp>

#include "constants.hpp"
#include "core/aperature.hpp"
#include "core/preset.hpp"
#include "core/transform.hpp"
#include "core/triangle_mesh.hpp"
#include "gfx/cpu/framebuffer.hpp"
#include "gfx/cpu/thread_pool.hpp"
#include "gfx/cpu/intersection.hpp"
#include "gfx/vk/triangle_mesh.hpp"
#include "ire/emitter.hpp"
#include "ire/uniform_layout.hpp"
#include "math_types.hpp"
#include "ire/core.hpp"

#include "glio.hpp"
#include "profiles/targets.hpp"

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
	layout_in <vec3, 0> position;
	layout_out <vec3, 0> color;

	push_constant <mvp> mvp;

	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;
	color = position;
}

void fragment()
{
	layout_in <vec3, 0> position;
	layout_out <vec4, 0> fragment;

	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
}

}

struct InteractiveWindow : littlevk::Window {
	InteractiveWindow() = default;

	InteractiveWindow(const littlevk::Window &win) : littlevk::Window(win) {}

	bool key_pressed(int key) const {
		return glfwGetKey(handle, key) == GLFW_PRESS;
	}
};

struct DeviceResourceCollection {
	vk::PhysicalDevice phdev;
	vk::SurfaceKHR surface;
	vk::Device device;
	vk::Queue graphics_queue;
	vk::Queue present_queue;
	vk::CommandPool command_pool;
	vk::DescriptorPool descriptor_pool;
	vk::PhysicalDeviceMemoryProperties memory_properties;

	littlevk::Swapchain swapchain;
	littlevk::Deallocator dal;

	InteractiveWindow window;

	auto allocator() {
		return littlevk::bind(device, memory_properties, dal);
	}

	auto combined() {
		return littlevk::bind(phdev, device);
	}

	auto commander() {
		return littlevk::bind(device, command_pool, graphics_queue);
	}

	template <typename ... Args>
	void configure_display(const Args &... args) {
		littlevk::Window win;
		std::tie(surface, win) = littlevk::surface_handles(args...);
		window = win;
	}

	void configure_device(const std::vector <const char *> &EXTENSIONS) {
		littlevk::QueueFamilyIndices queue_family = littlevk::find_queue_families(phdev, surface);

		memory_properties = phdev.getMemoryProperties();
		device = littlevk::device(phdev, queue_family, EXTENSIONS);
		dal = littlevk::Deallocator(device);

		graphics_queue = device.getQueue(queue_family.graphics, 0);
		present_queue = device.getQueue(queue_family.present, 0);

		command_pool = littlevk::command_pool(device,
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queue_family.graphics).unwrap(dal);

		swapchain = combined().swapchain(surface, queue_family);
	}
};

inline vk::DescriptorSet imgui_add_vk_texture(const vk::Sampler &sampler, const vk::ImageView &view, const vk::ImageLayout &layout)
{
	return ImGui_ImplVulkan_AddTexture(static_cast <VkSampler> (sampler),
			                   static_cast <VkImageView> (view),
					   static_cast <VkImageLayout> (layout));
}

inline void imgui_initialize_vulkan(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(drc.window.handle, true);

	vk::DescriptorPoolSize pool_size {
		vk::DescriptorType::eCombinedImageSampler, 1 << 10
	};

	auto descriptor_pool = littlevk::descriptor_pool(
		drc.device, vk::DescriptorPoolCreateInfo {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1 << 10, pool_size,
		}
	).unwrap(drc.dal);

	ImGui_ImplVulkan_InitInfo init_info = {};

	init_info.Instance = littlevk::detail::get_vulkan_instance();
	init_info.DescriptorPool = descriptor_pool;
	init_info.Device = drc.device;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.MinImageCount = 3;
	init_info.PhysicalDevice = drc.phdev;
	init_info.Queue = drc.graphics_queue;
	init_info.RenderPass = render_pass;

	ImGui_ImplVulkan_Init(&init_info);
}

struct FramebufferCollection {
	gfx::cpu::Framebuffer <uint32_t> host;
	littlevk::Buffer staging;
	littlevk::Image display;
	vk::DescriptorSet descriptor;
	vk::Sampler sampler;

	littlevk::LinkedDeviceAllocator <> allocator;
	littlevk::LinkedCommandQueue commander;

	FramebufferCollection(littlevk::LinkedDeviceAllocator <> allocator_,
				littlevk::LinkedCommandQueue commander_)
			: allocator(allocator_),
				commander(commander_) {
		sampler = littlevk::SamplerAssembler(allocator.device, allocator.dal);
	}

	void __copy(const vk::CommandBuffer &cmd) {
		display.transition(cmd, vk::ImageLayout::eTransferDstOptimal);

		littlevk::copy_buffer_to_image(cmd,
			display, staging,
			vk::ImageLayout::eTransferDstOptimal);

		display.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	void __allocate() {
		std::tie(staging, display) = allocator
			.buffer(host.data, vk::BufferUsageFlagBits::eTransferSrc)
			.image(vk::Extent2D {
					uint32_t(host.width),
					uint32_t(host.height)
				},
				vk::Format::eR8G8B8A8Unorm,
				vk::ImageUsageFlagBits::eSampled
					| vk::ImageUsageFlagBits::eTransferDst,
				vk::ImageAspectFlagBits::eColor);

		commander.submit_and_wait([&](const vk::CommandBuffer &cmd) {
			__copy(cmd);
		});

		descriptor = imgui_add_vk_texture(sampler, display.view, vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	void resize(size_t width, size_t height) {
		host.resize(width, height);
		__allocate();
	}

	void refresh(const vk::CommandBuffer &cmd) {
		littlevk::upload(allocator.device, staging, host.data);
		__copy(cmd);
	}
};

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::filesystem::path path = argv[1];
	fmt::println("path to scene: {}", path);

	auto preset = core::Preset::from(path).value();

	//////////////////
	// VULKAN SETUP //
	//////////////////

	// Device extensions
	static const std::vector <const char *> EXTENSIONS {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	DeviceResourceCollection drc;

	drc.phdev = littlevk::pick_physical_device(predicate);
	drc.configure_display(vk::Extent2D(1920, 1080), "javelin");
	drc.configure_device(EXTENSIONS);

	vk::RenderPass ui_render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	imgui_initialize_vulkan(drc, ui_render_pass);

	std::vector <vk::Framebuffer> ui_framebuffers;
	std::vector <vk::Framebuffer> framebuffers;
	std::vector <littlevk::Image> render_color_targets;

	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	std::vector <core::TriangleMesh> tmeshes;
	std::vector <gfx::vulkan::TriangleMesh> vmeshes;

	for (size_t i = 0; i < preset.geometry.size(); i++) {
		auto tmesh = core::TriangleMesh::from(preset.geometry[i]).value();
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
		.with_render_pass(render_pass, 0)
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

	core::Aperature primary_aperature;
	core::Aperature local_aperature;
	core::Transform transform;

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	MVP mvp;
	mvp.model = float4x4::identity();
	mvp.proj = core::perspective(primary_aperature);

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
		littlevk::FramebufferGenerator generator(drc.device, render_pass, drc.window.extent, drc.dal);

		for (size_t i = 0; i < drc.swapchain.images.size(); i++) {
			ui_generator.add(drc.swapchain.image_views[i]);
			generator.add(render_color_targets[i].view, depth.view);
		}

		ui_framebuffers = ui_generator.unpack();
		framebuffers = generator.unpack();

		export_render_targets_to_imgui();

		auto transition = [&](const vk::CommandBuffer &cmd) {
			for (auto &image : render_color_targets)
				image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
		};

		drc.commander().submit_and_wait(transition);
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

	auto fb = FramebufferCollection(drc.allocator(), drc.commander());
	fb.resize(200, 200);
	fb.host.clear();

	auto rf = core::rayframe(primary_aperature, transform);

	auto tonemap = [](const float3 &rgb) -> uint32_t {
		// TODO: r g b a components with a union
		uint32_t r = 255 * rgb.x;
		uint32_t g = 255 * rgb.y;
		uint32_t b = 255 * rgb.z;
		return r | g << 8 | b << 16 | 0xff000000;
	};

	auto raytrace = [&tmeshes, &rf, &tonemap](int2 ij, float2 uv) -> uint32_t {
		float3 origin = rf.origin;
		float3 ray = normalize(rf.lower_left
				+ uv.x * rf.horizontal
				+ (1 - uv.y) * rf.vertical
				- rf.origin);

		gfx::cpu::Intersection sh = gfx::cpu::Intersection::miss();
		for (const auto &tmesh : tmeshes) {
			for (const int3 &t : tmesh.triangles) {
				float3 v0 = tmesh.positions[t.x];
				float3 v1 = tmesh.positions[t.y];
				float3 v2 = tmesh.positions[t.z];

				auto hit = gfx::cpu::ray_triangle_intersection(ray, origin, v0, v1, v2);
				sh.update(hit);
			}
		}

		float3 color;
		if (sh.time >= 0)
			color = 0.5f + 0.5f * sh.normal;

		return tonemap(color);
	};

	auto clear = [&fb](int2 ij, float2 uv) -> uint32_t {
		return 0xff000000;
	};

	int2 size = { 16, 16 };
	auto tiles = fb.host.tiles(size);

	auto kernel = [&fb, &raytrace](const gfx::cpu::Tile &tile) {
		fb.host.process_tile(raytrace, tile);
	};

	auto thread_pool = gfx::cpu::fixed_function_thread_pool <gfx::cpu::Tile, decltype(kernel)> (8, kernel);

	// TODO: dynamic render pass
	ImVec2 viewport;

	uint32_t frame = 0;
        while (true) {
		handle_input();

		// TODO: method instead of function
		mvp.proj = core::perspective(primary_aperature);
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
			(render_pass, framebuffers[op.index], drc.window);

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

			if (ImGui::Begin("Interface")) {
				ImGui::End();
			}

			bool preview_open = true;
			if (ImGui::Begin("Preview", &preview_open, ImGuiWindowFlags_MenuBar)) {
				if (ImGui::BeginMenuBar()) {
					if (ImGui::Button("Render")) {
						fb.resize(viewport.x, viewport.y);
						fmt::println("rendering at {} x {} resolution", fb.host.width, fb.host.height);

						auto tiles = fb.host.tiles(size);

						rf = core::rayframe(local_aperature, transform);

						thread_pool.reset();
						thread_pool.enque(tiles);
						fb.host.process(clear);
						thread_pool.begin();
					}

					ImGui::EndMenuBar();
				}

				viewport_region.active = ImGui::IsWindowFocused();

				ImVec2 size = ImGui::GetContentRegionAvail();
				ImGui::Image(imgui_descriptors[frame], size);

				ImVec2 viewport = ImGui::GetItemRectSize();
				primary_aperature.aspect = viewport.x/viewport.y;

				ImVec2 min = ImGui::GetItemRectMin();
				ImVec2 max = ImGui::GetItemRectMax();

				viewport_region.min = { min.x, min.y };
				viewport_region.max = { max.x, max.y };

				ImGui::End();
			}

			if (ImGui::Begin("Framebuffer")) {
				viewport = ImGui::GetContentRegionAvail();
				local_aperature.aspect = viewport.x/viewport.y;
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

	thread_pool.reset();
}
