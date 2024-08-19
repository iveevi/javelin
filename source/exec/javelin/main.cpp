#include <cassert>
#include <csignal>
#include <cstdlib>
#include <filesystem>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>
#include <random>

// JVL headers
#include "constants.hpp"
#include "core/aperature.hpp"
#include "core/preset.hpp"
#include "core/transform.hpp"
#include "core/triangle_mesh.hpp"
#include "gfx/cpu/bvh.hpp"
#include "gfx/cpu/framebuffer.hpp"
#include "gfx/cpu/intersection.hpp"
#include "gfx/cpu/scene.hpp"
#include "gfx/cpu/thread_pool.hpp"
#include "gfx/vk/triangle_mesh.hpp"
#include "ire/callable.hpp"
#include "ire/core.hpp"
#include "ire/emitter.hpp"
#include "ire/uniform_layout.hpp"
#include "math_types.hpp"

// Local project headers
#include "glio.hpp"
#include "device_resource_collection.hpp"
#include "host_framebuffer_collection.hpp"

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

using namespace jvl::core;
using namespace jvl::gfx;

inline float3 reflect(const float3 &v, const float3 &n)
{
    return normalize(v - 2 * dot(v, n) * n);
}

inline std::pair <float3, float> sample_hemisphere(float u, float v)
{
	assert(u >= 0 && u < 1);
	assert(v >= 0 && v < 1);

	float phi = 2 * M_PI * u;
	float z = sqrt(1 - v);

	return {
		float3 {
			std::cos(phi) * std::sqrt(v),
			std::sin(phi) * std::sqrt(v),
			z
		},
		z/M_PI
	};
}

struct OrthonormalBasis {
	float3 u, v, w;

	float3 local(const float3 &a) const {
		return a.x * u + a.y * v + a.z * w;
	}

	static OrthonormalBasis from(const float3 &n) {
		OrthonormalBasis onb;
		onb.w = normalize(n);

		float3 a = (fabs(onb.w.x) > 0.9) ? float3 { 0, 1, 0 } : float3 { 1, 0, 0 };
		onb.v = normalize(cross(onb.w, a));
		onb.u = cross(onb.w, onb.v);

		return onb;
	}
};

struct HostRaytracer {
	core::Transform transform;
	core::Aperature aperature;
	core::Rayframe rayframe;

	cpu::Scene &scene;

	// TODO: thread_local everything
	std::mt19937 random;
	std::uniform_real_distribution <float> distribution;

	HostRaytracer(cpu::Scene &scene_)
			: scene(scene_),
			  random(std::random_device()()),
	                  distribution(0, 1) {}

	uint32_t convert_format(const float3 &rgb) {
		// TODO: r g b a components with a union
		uint32_t r = 255 * rgb.x;
		uint32_t g = 255 * rgb.y;
		uint32_t b = 255 * rgb.z;
		return r | g << 8 | b << 16 | 0xff000000;
	}

	float3 radiance(const Ray &ray, int depth = 0) {
		if (depth <= 0)
			return float3(0.0f);

		auto sh = scene.trace(ray);

		// TODO: get environment map lighting from scene...
		if (sh.time <= 0.0)
			return float3(0.0f);

		assert(sh.material < scene.materials.size());
		Material &material = scene.materials[sh.material];

		Phong phong = Phong::from(material);
		float3 emission = phong.emission.as <float3> ();
		if (length(emission) > 0)
			return emission;

		float3 kd = phong.kd.as <float3> ();

		constexpr float epsilon = 1e-3f;

		float u = distribution(random);
		float v = distribution(random);

		auto [sampled, pdf] = sample_hemisphere(u, v);

		auto onb = OrthonormalBasis::from(sh.normal);

		Ray next = ray;
		next.origin = sh.point + epsilon * sh.normal;
		next.direction = onb.local(sampled);

		float lambertian = std::max(dot(sh.normal, next.direction), 0.0f);
		float3 brdf = lambertian * (kd/pi <float>);
		float3 Le = radiance(next, depth - 1);
		float3 ret = Le * kd;
		return brdf * Le/pdf;
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
};

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::filesystem::path path = argv[1];
	fmt::println("path to scene: {}", path);

	auto preset = core::Preset::from(path).value();

	// TODO: non blocking bvh construction (std promise equivalent?)
	auto scene = cpu::Scene();
	scene.add(preset);
	scene.build_bvh();

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

	auto fb = HostFramebufferCollection(drc.allocator(), drc.commander());

	HostRaytracer rtx(scene);

	auto raytrace = [&](int2 ij, float2 uv) -> uint32_t {
		Ray ray = rtx.ray_from_pixel(uv);
		float3 color = rtx.radiance_samples(ray, 64);
		color = clamp(color, 0, 1);
		return rtx.convert_format(color);
	};

	auto clear = [&fb](int2 ij, float2 uv) -> uint32_t {
		return 0xff000000;
	};

	auto kernel = [&fb, &raytrace](const gfx::cpu::Tile &tile) {
		fb.host.process_tile(raytrace, tile);
	};

	auto thread_pool = gfx::cpu::fixed_function_thread_pool
			<gfx::cpu::Tile, decltype(kernel)>
			(std::thread::hardware_concurrency(), kernel);

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

						int2 size { 32, 32 };
						auto tiles = fb.host.tiles(size);

						rtx.transform = transform;
						rtx.rayframe = core::rayframe(rtx.aperature, rtx.transform);

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

	thread_pool.reset();
}
