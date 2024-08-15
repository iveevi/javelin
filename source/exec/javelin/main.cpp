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

	auto layout() {
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

inline vk::DescriptorSet imgui_add_vk_texture(const vk::Sampler &sampler, const vk::ImageView &view, const vk::ImageLayout &layout)
{
	return ImGui_ImplVulkan_AddTexture(static_cast <VkSampler> (sampler),
			                   static_cast <VkImageView> (view),
					   static_cast <VkImageLayout> (layout));
}

std::vector <std::function <void ()>> terminal;

void signal_handler(int)
{
	fmt::println("signal handler!");
	for (const auto &t : terminal)
		t();

	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	// Signal handlers
	// std::signal(SIGINT, signal_handler);
	// std::signal(SIGKILL, signal_handler);

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

	vk::PhysicalDevice phdev = littlevk::pick_physical_device(predicate);

	vk::SurfaceKHR surface;
	littlevk::Window window;

	vk::Extent2D default_extent { 1920, 1080 };

	std::tie(surface, window) = littlevk::surface_handles(default_extent, "javelin");

	littlevk::QueueFamilyIndices queue_family = littlevk::find_queue_families(phdev, surface);

	auto memory_properties = phdev.getMemoryProperties();
	auto device = littlevk::device(phdev, queue_family, EXTENSIONS);
	auto dal = littlevk::Deallocator(device);

	auto allocator = littlevk::bind(device, memory_properties, dal);
	auto combined = littlevk::bind(phdev, device);

	auto swapchain = combined.swapchain(surface, queue_family);

	auto graphics_queue = device.getQueue(queue_family.graphics, 0);
	auto present_queue = device.getQueue(queue_family.present, 0);

	vk::RenderPass ui_render_pass = littlevk::RenderPassAssembler(device, dal)
		.add_attachment(littlevk::default_color_attachment(swapchain.format))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	vk::RenderPass render_pass = littlevk::RenderPassAssembler(device, dal)
		.add_attachment(littlevk::default_color_attachment(swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	std::vector <vk::Framebuffer> ui_framebuffers;
	std::vector <vk::Framebuffer> framebuffers;

	std::vector <littlevk::Image> render_color_targets;

	{
		littlevk::Image depth = allocator
			.image(window.extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		render_color_targets.clear();
		for (size_t i = 0; i < swapchain.images.size(); i++) {
			render_color_targets.push_back(allocator
				.image(window.extent,
					vk::Format::eB8G8R8A8Unorm,
					vk::ImageUsageFlagBits::eColorAttachment
						| vk::ImageUsageFlagBits::eSampled,
					vk::ImageAspectFlagBits::eColor)
			);
		}

		littlevk::FramebufferGenerator ui_generator(device, ui_render_pass, window.extent, dal);
		littlevk::FramebufferGenerator generator(device, render_pass, window.extent, dal);

		for (size_t i = 0; i < swapchain.images.size(); i++) {
			ui_generator.add(swapchain.image_views[i]);
			generator.add(render_color_targets[i].view, depth.view);
		}

		ui_framebuffers = ui_generator.unpack();
		framebuffers = generator.unpack();
	}

	auto command_pool = littlevk::command_pool(device,
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family.graphics).unwrap(dal);

	auto command_buffers = littlevk::command_buffers(device,
		command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	std::vector <core::TriangleMesh> tmeshes;
	std::vector <gfx::vulkan::TriangleMesh> vmeshes;

	for (size_t i = 0; i < preset.geometry.size(); i++) {
		auto tmesh = core::TriangleMesh::from(preset.geometry[i]).value();
		tmeshes.push_back(tmesh);

		auto vmesh = gfx::vulkan::TriangleMesh::from(allocator, tmesh).value();
		vmeshes.push_back(vmesh);
	}

	// Create a graphics pipeline
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	std::string vertex_shader = ire::kernel_from_args(vertex).synthesize(profiles::opengl_450);
	std::string fragment_shader = ire::kernel_from_args(fragment).synthesize(profiles::opengl_450);

	auto bundle = littlevk::ShaderStageBundle(device, dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	littlevk::Pipeline ppl = littlevk::PipelineAssembler <littlevk::eGraphics> (device, window, dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <MVP> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);

	// Syncronization primitives
	auto sync = littlevk::present_syncronization(device, 2).unwrap(dal);

	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.handle, true);

	vk::DescriptorPoolSize pool_size {
		vk::DescriptorType::eCombinedImageSampler, 1 << 10
	};

	auto descriptor_pool = littlevk::descriptor_pool(
		device, vk::DescriptorPoolCreateInfo {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1 << 10, pool_size,
		}
	).unwrap(dal);

	ImGui_ImplVulkan_InitInfo init_info = {};

	init_info.Instance = littlevk::detail::get_vulkan_instance();
	init_info.DescriptorPool = descriptor_pool;
	init_info.Device = device;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.MinImageCount = 3;
	init_info.PhysicalDevice = phdev;
	init_info.Queue = graphics_queue;
	init_info.RenderPass = ui_render_pass;

	ImGui_ImplVulkan_Init(&init_info);

	std::vector <vk::DescriptorSet> imgui_descriptors;

	auto export_render_targets_to_imgui = [&]() {
		imgui_descriptors.clear();

		vk::Sampler sampler = littlevk::SamplerAssembler(device, dal);
		for (const littlevk::Image &image : render_color_targets) {
			vk::DescriptorSet dset = imgui_add_vk_texture(sampler, image.view,
					vk::ImageLayout::eShaderReadOnlyOptimal);

			imgui_descriptors.push_back(dset);
		}
	};

	export_render_targets_to_imgui();

	core::Aperature aperature;
	core::Transform transform;

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	MVP mvp;
	mvp.model = float4x4::identity();
	mvp.proj = core::perspective(aperature);

	auto resize = [&]() {
		combined.resize(surface, window, swapchain);

		littlevk::Image depth = allocator
			.image(window.extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		render_color_targets.clear();
		for (size_t i = 0; i < swapchain.images.size(); i++) {
			render_color_targets.push_back(allocator
				.image(window.extent,
					vk::Format::eB8G8R8A8Unorm,
					vk::ImageUsageFlagBits::eColorAttachment
						| vk::ImageUsageFlagBits::eSampled,
					vk::ImageAspectFlagBits::eColor)
			);
		}

		littlevk::FramebufferGenerator ui_generator(device, ui_render_pass, window.extent, dal);
		littlevk::FramebufferGenerator generator(device, render_pass, window.extent, dal);

		for (size_t i = 0; i < swapchain.images.size(); i++) {
			ui_generator.add(swapchain.image_views[i]);
			generator.add(render_color_targets[i].view, depth.view);
		}

		ui_framebuffers = ui_generator.unpack();
		framebuffers = generator.unpack();

		export_render_targets_to_imgui();

		auto transition = [&](const vk::CommandBuffer &cmd) {
			for (auto &image : render_color_targets)
				image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
		};

		// TODO: linked queue pool
		littlevk::submit_now(device, command_pool, graphics_queue, transition);

		aperature.aspect = float(window.extent.width)/float(window.extent.height);
		mvp.proj = core::perspective(aperature);
	};

	auto handle_input = [&]() {
		static float last_time = 0.0f;

		constexpr float speed = 50.0f;

		float delta = speed * float(glfwGetTime() - last_time);
		last_time = glfwGetTime();

		float3 velocity(0.0f);
		if (glfwGetKey(window.handle, GLFW_KEY_S) == GLFW_PRESS)
			velocity.z -= delta;
		else if (glfwGetKey(window.handle, GLFW_KEY_W) == GLFW_PRESS)
			velocity.z += delta;

		if (glfwGetKey(window.handle, GLFW_KEY_D) == GLFW_PRESS)
			velocity.x -= delta;
		else if (glfwGetKey(window.handle, GLFW_KEY_A) == GLFW_PRESS)
			velocity.x += delta;

		if (glfwGetKey(window.handle, GLFW_KEY_E) == GLFW_PRESS)
			velocity.y += delta;
		else if (glfwGetKey(window.handle, GLFW_KEY_Q) == GLFW_PRESS)
			velocity.y -= delta;

		transform.translate += transform.rotation.rotate(velocity);
	};

	glfwSetWindowUserPointer(window.handle, &transform);
	glfwSetMouseButtonCallback(window.handle, button_callback);
	glfwSetCursorPosCallback(window.handle, cursor_callback);

	auto fb = gfx::cpu::Framebuffer <uint32_t> ::from(200, 200);
	fb.clear();

	auto rf = core::rayframe(aperature, transform);

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
	auto tiles = fb.tiles(size);

	auto kernel = [&fb, &raytrace](const gfx::cpu::Tile &tile) {
		fb.process_tile(raytrace, tile);
	};

	using thread_pool_t = gfx::cpu::fixed_function_thread_pool <gfx::cpu::Tile, decltype(kernel)>;
	thread_pool_t thread_pool(std::thread::hardware_concurrency(), kernel);

	littlevk::Buffer fb_buffer;
	littlevk::Image fb_image;

	std::tie(fb_buffer, fb_image) = allocator
		.buffer(fb.data, vk::BufferUsageFlagBits::eTransferSrc)
		.image(vk::Extent2D { fb.width, fb.height },
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageUsageFlagBits::eSampled
				| vk::ImageUsageFlagBits::eTransferDst,
			vk::ImageAspectFlagBits::eColor);

	littlevk::bind(device, command_pool, graphics_queue)
		.submit_and_wait([&](const vk::CommandBuffer &cmd) {
			fb_image.transition(cmd, vk::ImageLayout::eTransferDstOptimal);
			littlevk::copy_buffer_to_image(cmd, fb_image, fb_buffer, vk::ImageLayout::eTransferDstOptimal);
			fb_image.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
		});

	vk::Sampler sampler = littlevk::SamplerAssembler(device, dal);
	vk::DescriptorSet cpu_fb_dset = imgui_add_vk_texture(sampler, fb_image.view,
			vk::ImageLayout::eShaderReadOnlyOptimal);

	terminal.push_back([&]() { thread_pool.reset(); });

	// TODO: dynamic render pass
	ImVec2 viewport;

	uint32_t frame = 0;
        while (true) {
		handle_input();

		// TODO: method instead of function
		mvp.proj = core::perspective(aperature);
		mvp.view = transform.to_view_matrix();

                glfwPollEvents();
                if (glfwWindowShouldClose(window.handle))
                        break;

		littlevk::SurfaceOperation op;
                op = littlevk::acquire_image(device, swapchain.swapchain, sync[frame]);
		if (op.status == littlevk::SurfaceOperation::eResize) {
			resize();
			continue;
		}

		// Start the render pass
		const auto &cmd = command_buffers[frame];
		cmd.begin(vk::CommandBufferBeginInfo {});

		// Set viewport and scissor
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(window));

		const auto &render_rpbi = littlevk::default_rp_begin_info <2>
			(render_pass, framebuffers[op.index], window);

		// Update framebuffer buffer and copy to the image
		littlevk::upload(device, fb_buffer, fb.data);

		fb_image.transition(cmd, vk::ImageLayout::eTransferDstOptimal);
		littlevk::copy_buffer_to_image(cmd, fb_image, fb_buffer, vk::ImageLayout::eTransferDstOptimal);
		fb_image.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);

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
			(ui_render_pass, ui_framebuffers[op.index], window);

		cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

			if (ImGui::Begin("Interface")) {
				if (ImGui::Button("Render")) {
					fb.resize(viewport.x, viewport.y);
					fmt::println("rendering at {} x {} resolution", fb.width, fb.height);

					std::tie(fb_buffer, fb_image) = allocator
						.buffer(fb.data, vk::BufferUsageFlagBits::eTransferSrc)
						.image(vk::Extent2D { fb.width, fb.height },
								vk::Format::eR8G8B8A8Unorm,
								vk::ImageUsageFlagBits::eSampled
								| vk::ImageUsageFlagBits::eTransferDst,
								vk::ImageAspectFlagBits::eColor);

					littlevk::bind(device, command_pool, graphics_queue)
						.submit_and_wait([&](const vk::CommandBuffer &cmd) {
								fb_image.transition(cmd, vk::ImageLayout::eTransferDstOptimal);
								littlevk::copy_buffer_to_image(cmd, fb_image, fb_buffer, vk::ImageLayout::eTransferDstOptimal);
								fb_image.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
								});

					vk::Sampler sampler = littlevk::SamplerAssembler(device, dal);
					cpu_fb_dset = imgui_add_vk_texture(sampler, fb_image.view,
							vk::ImageLayout::eShaderReadOnlyOptimal);

					auto tiles = fb.tiles(size);

					rf = core::rayframe(aperature, transform);

					thread_pool.reset();
					thread_pool.enque(tiles);
					fb.process(clear);
					thread_pool.begin();
				}

				ImGui::End();
			}

			if (ImGui::Begin("Preview")) {
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

			if (ImGui::Begin("Framebuffer")) {
				viewport = ImGui::GetContentRegionAvail();
				ImGui::Image(cpu_fb_dset, viewport);
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

		graphics_queue.submit(submit_info, sync.in_flight[frame]);

                op = littlevk::present_image(present_queue, swapchain.swapchain, sync[frame], op.index);
		if (op.status == littlevk::SurfaceOperation::eResize)
			resize();

		frame = 1 - frame;
        }

	device.waitIdle();

	window.drop();

	dal.drop();

	thread_pool.reset();
}
