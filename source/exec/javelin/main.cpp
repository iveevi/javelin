#include <cassert>
#include <csignal>
#include <cstdlib>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>

#include "engine/imported_asset.hpp"
#include "core/scene.hpp"
#include "core/transform.hpp"
#include "core/aperature.hpp"
#include "gfx/vk/scene.hpp"
#include "ire/core.hpp"

// Local project headers
#include "device_resource_collection.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::ire;
using namespace jvl::gfx;

struct RenderingInfo {
	const vk::CommandBuffer &cmd;
	const littlevk::SurfaceOperation &operation;
	const InteractiveWindow &window;
	int32_t frame;
};

// Renders to the swapchain images
struct ImGuiRenderGroup {
	vk::RenderPass render_pass;
	std::vector <vk::Framebuffer> framebuffers;

	auto render_pass_begin_info(const RenderingInfo &info) const {
		return littlevk::default_rp_begin_info <1> (render_pass,
			framebuffers[info.operation.index], info.window);
	}

	void resize(DeviceResourceCollection &drc) {
		auto &swapchain = drc.swapchain;
		
		littlevk::FramebufferGenerator generator {
			drc.device, render_pass,
			drc.window.extent, drc.dal
		};

		for (size_t i = 0; i < swapchain.images.size(); i++)
			generator.add(swapchain.image_views[i]);

		framebuffers = generator.unpack();
	}

	static ImGuiRenderGroup from(DeviceResourceCollection &drc) {
		ImGuiRenderGroup prg;

		prg.render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
			.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.done();

		prg.resize(drc);	

		return prg;
	}
};

using imgui_callback = std::function <void ()>;
using imgui_callback_list = std::vector <imgui_callback>;

void render_imgui(const RenderingInfo &info, const ImGuiRenderGroup &irg, const imgui_callback_list &callbacks)
{
	// Configure the rendering extent
	littlevk::viewport_and_scissor(info.cmd, littlevk::RenderArea(info.window));

	// Start the rendering pass
	auto rpbi = irg.render_pass_begin_info(info);
	info.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
	
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
	
	for (auto &callback : callbacks)
		callback();

	// Complete the rendering	
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), info.cmd);
	
	// Finish the rendering pass
	info.cmd.endRenderPass();
}

enum ViewportMode : int32_t {
	eAlbedo,
	eNormal,
	eDepth,
	eCount,
};

struct Viewport {
	// Viewing information
	jvl::core::Aperature aperature;
	jvl::core::Transform transform;
	ViewportMode mode = eNormal;

	void handle_input(const InteractiveWindow &window) {
		static constexpr float speed = 50.0f;
		static float last_time = 0.0f;
		
		float delta = speed * float(glfwGetTime() - last_time);
		last_time = glfwGetTime();

		jvl::float3 velocity(0.0f);
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
};

// Separated from ViewportRenderGroup because we can have
// multiple viewports using the exact same render pass and pipelines
struct ViewportFramebuffers {
	std::vector <littlevk::Image> targets;
	std::vector <vk::Framebuffer> framebuffers;
	std::vector <vk::DescriptorSet> imgui_descriptors;
	vk::Extent2D extent;

	void resize(DeviceResourceCollection &drc, const vk::RenderPass &render_pass) {
		targets.clear();
		framebuffers.clear();
		imgui_descriptors.clear();

		littlevk::Image depth = drc.allocator()
			.image(extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		targets.clear();
		for (size_t i = 0; i < drc.swapchain.images.size(); i++) {
			targets.push_back(drc.allocator()
				.image(extent,
					vk::Format::eB8G8R8A8Unorm,
					vk::ImageUsageFlagBits::eColorAttachment
						| vk::ImageUsageFlagBits::eSampled,
					vk::ImageAspectFlagBits::eColor)
			);
		}

		littlevk::FramebufferGenerator generator {
			drc.device, render_pass,
			extent, drc.dal
		};

		for (size_t i = 0; i < drc.swapchain.images.size(); i++)
			generator.add(targets[i].view, depth.view);

		framebuffers = generator.unpack();

		vk::Sampler sampler = littlevk::SamplerAssembler(drc.device, drc.dal);
		for (const littlevk::Image &image : targets) {
			vk::DescriptorSet dset = imgui::add_vk_texture(sampler, image.view,
				vk::ImageLayout::eShaderReadOnlyOptimal);

			imgui_descriptors.push_back(dset);
		}

		auto transition = [&](const vk::CommandBuffer &cmd) {
			for (auto &image : targets)
				image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
		};

		drc.commander().submit_and_wait(transition);
	}

	static ViewportFramebuffers from(DeviceResourceCollection &drc, const vk::RenderPass &render_pass) {
		ViewportFramebuffers vf;
		vf.extent = drc.window.extent;
		vf.resize(drc, render_pass);
		return vf;
	}
};

struct ViewportBundle {
	Viewport info;
	ViewportFramebuffers fb;

	// TODO: generate the callback from here...

	static ViewportBundle from(DeviceResourceCollection &drc, const vk::RenderPass &render_pass) {
		ViewportBundle vb;
		vb.fb = ViewportFramebuffers::from(drc, render_pass);
		return vb;
	}
};

// Shader sources
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

void vertex()
{
	layout_in <vec3> position(0);
	layout_out <vec3> color(0);

	push_constant <MVP> mvp;

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

class ViewportRenderGroup {
	void configure_render_pass(DeviceResourceCollection &drc) {
		render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();
	}

	void configure_normal_pipeline(DeviceResourceCollection &drc) {
		auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

		std::string vertex_shader = kernel_from_args(vertex).compile(profiles::glsl_450);
		std::string fragment_shader = kernel_from_args(fragment).compile(profiles::glsl_450);

		auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
			.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

		pipelines[eNormal] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
			.with_render_pass(render_pass, 0)
			.with_vertex_layout(vertex_layout)
			.with_shader_bundle(bundle)
			.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
			.cull_mode(vk::CullModeFlagBits::eNone);
	}

	void configure_pipelines(DeviceResourceCollection &drc) {
		configure_normal_pipeline(drc);
	}
public:
	vk::RenderPass render_pass;

	// Pipeline kinds
	std::array <littlevk::Pipeline, eCount> pipelines;

	auto render_pass_begin_info(const RenderingInfo &info, const ViewportFramebuffers &fbs) const {
		return littlevk::default_rp_begin_info <2> (render_pass,
			fbs.framebuffers[info.operation.index],
			fbs.extent);
	}

	static ViewportRenderGroup from(DeviceResourceCollection &drc) {
		ViewportRenderGroup vrg;
		vrg.configure_render_pass(drc);
		vrg.configure_pipelines(drc);
		return vrg;
	}
};

void render_viewport_scene(const RenderingInfo &info,
			   const ViewportRenderGroup &vrg,
			   ViewportBundle &viewport,
			   const vulkan::Scene &scene)
{
	// Configure the rendering extent
	littlevk::viewport_and_scissor(info.cmd, littlevk::RenderArea(viewport.fb.extent));

	viewport.info.handle_input(info.window);

	auto rpbi = vrg.render_pass_begin_info(info, viewport.fb);
	info.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

	auto &ppl = vrg.pipelines[viewport.info.mode];

	info.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

	solid_t <MVP> mvp;

	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(viewport.info.aperature);
	mvp[m_view] = viewport.info.transform.to_view_matrix();

	info.cmd.pushConstants <solid_t <MVP>> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);

	for (const auto &vmesh : scene.meshes) {
		info.cmd.bindVertexBuffers(0, vmesh.vertices.buffer, { 0 });
		info.cmd.bindIndexBuffer(vmesh.triangles.buffer, 0, vk::IndexType::eUint32);
		info.cmd.drawIndexed(vmesh.count, 1, 0, 0, 0);
	}

	info.cmd.endRenderPass();

	littlevk::transition(info.cmd, viewport.fb.targets[info.frame],
		vk::ImageLayout::ePresentSrcKHR,
		vk::ImageLayout::eShaderReadOnlyOptimal);
}

class WindowEventSystem {
	double last_x = 0.0;
	double last_y = 0.0;
	int64_t uuid_counter = 0;
public:
	struct button_event {
		double x;
		double y;
		int button;
		int action;
		int mods;
	};

	struct cursor_event {
		double x;
		double y;
		double dx;
		double dy;
		bool drag;
	};

	using cursor_event_callback = std::function <void (const cursor_event &)>;

	class event_region {
		float2 min;
		float2 max;
		bool drag = false;

		std::optional <cursor_event_callback> cursor_callback;
	public:
		void set_cursor_callback(const cursor_event_callback &cbl) {
			cursor_callback = cbl;
		}

		void set_bounds(const ImVec2 &min_, const ImVec2 &max_) {
			min = { min_.x, min_.y };
			max = { max_.x, max_.y };
		}

		bool contains(double x, double y) const {
			return (x > min.x && x < max.x)
				&& (y > min.y && y < max.y);
		}

		friend class WindowEventSystem;
	};

	wrapped::hash_table <int64_t, event_region> regions;

	int64_t new_uuid() {
		int64_t ret = uuid_counter++;
		regions[ret] = event_region();
		return ret;
	}

	void button_callback(GLFWwindow *window, int button, int action, int mods) {
		button_event event {
			.button = button,
			.action = action,
			.mods = mods,
		};

		glfwGetCursorPos(window, &event.x, &event.y);

		bool held = false;
		for (auto &[uuid, region] : regions) {
			if (region.contains(event.x, event.y)) {
				if (action == GLFW_PRESS)
					region.drag = true;
				else if (action == GLFW_RELEASE)
					region.drag = false;

				held = true;
			} else {
				region.drag = false;
			}
		}

		if (held)
			return;
		
		ImGuiIO &io = ImGui::GetIO();
		io.AddMouseButtonEvent(button, action);
	}

	void cursor_callback(GLFWwindow *window, double x, double y) {
		cursor_event event {
			.x = x,
			.y = y,
			.dx = x - last_x,
			.dy = y - last_y,
		};

		last_x = x;
		last_y = y;

		for (auto &[uuid, region] : regions) {
			if (region.contains(event.x, event.y)) {
				auto re = event;
				re.drag = region.drag;

				if (region.cursor_callback)
					region.cursor_callback.value()(re);
			}
		}
		
		ImGuiIO &io = ImGui::GetIO();
		io.MousePos = ImVec2(x, y);
	}
};

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto user = glfwGetWindowUserPointer(window);
	auto event_system = reinterpret_cast <WindowEventSystem *> (user);
	event_system->button_callback(window, button, action, mods);
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	auto user = glfwGetWindowUserPointer(window);
	auto event_system = reinterpret_cast <WindowEventSystem *> (user);
	event_system->cursor_callback(window, x, y);
}

struct Editor {
	DeviceResourceCollection	drc;

	// Unique render groups
	ImGuiRenderGroup		rg_imgui;
	ViewportRenderGroup		rg_viewport;

	// Scene management
	core::Scene			scene;
	cpu::Scene			host_scene;
	vulkan::Scene			vk_scene;

	// Supporting multiple viewports
	std::vector <ViewportBundle>	viewports;
};

int main(int argc, char *argv[])
{
	littlevk::config().enable_logging = false;
	littlevk::config().abort_on_validation_error = true;

	assert(argc >= 2);

	// Load the asset and scene
	std::filesystem::path path = argv[1];

	auto asset = engine::ImportedAsset::from(path).value();
	auto scene = core::Scene();
	scene.add(asset);

	// Device extensions
	static const std::vector <const char *> EXTENSIONS {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	DeviceResourceCollectionInfo info {
		.phdev = littlevk::pick_physical_device(predicate),
		.title = "Editor",
		.extent = vk::Extent2D(1920, 1080),
		.extensions = EXTENSIONS,
	};
	
	auto drc = DeviceResourceCollection::from(info);
	auto rg_imgui = ImGuiRenderGroup::from(drc);

	imgui::configure_vulkan(drc, rg_imgui.render_pass);
	
	auto rg_viewport = ViewportRenderGroup::from(drc);
	auto viewport = ViewportBundle::from(drc, rg_viewport.render_pass);

	// Prepare host and device scenes
	auto cpu_scene = cpu::Scene::from(scene);
	auto vk_scene = vulkan::Scene::from(drc.allocator(), cpu_scene);

	// Event system(s)
	WindowEventSystem event_system;

	glfwSetWindowUserPointer(drc.window.handle, &event_system);
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);
	
	// Synchronization information
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	// Main loop
	struct {
		int index = 0;
	} frame;

	auto main_menu_bar = []() {
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Attachments")) {
			if (ImGui::MenuItem("Viewport")) {
			}
			
			if (ImGui::MenuItem("Raytracer (CPU)")) {

			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	};

	int64_t viewport_uuid = event_system.new_uuid();

	auto viewport_cursor_callback = [&](const WindowEventSystem::cursor_event &event) {
		if (!event.drag)
			return;

		static constexpr float sensitivity = 0.0025f;
		static float pitch = 0;
		static float yaw = 0;
		
		pitch -= event.dx * sensitivity;
		yaw += event.dy * sensitivity;
		
		float pi_e = pi <float> / 2.0f - 1e-3f;
		yaw = std::min(pi_e, std::max(-pi_e, yaw));

		auto &transform = viewport.info.transform;
		transform.rotation = fquat::euler_angles(yaw, pitch, 0);
	};

	event_system.regions[viewport_uuid].set_cursor_callback(viewport_cursor_callback);

	auto display_viewport = [&]() {
		bool preview_open = true;
		ImGui::Begin("Viewport", &preview_open, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::MenuItem("Render")) {
				fmt::println("triggering cpu raytracer...");
			}

			ImGui::EndMenuBar();
		}

		ImVec2 size;
		size = ImGui::GetContentRegionAvail();
		ImGui::Image(viewport.fb.imgui_descriptors[frame.index], size);

		size = ImGui::GetItemRectSize();
		viewport.info.aperature.aspect = size.x/size.y;

		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();

		auto &region = event_system.regions[viewport_uuid];
		region.set_bounds(min, max);

		ImGui::GetForegroundDrawList()->AddRect(min, max, IM_COL32(255, 0, 0, 255));

		ImGui::End();
	};

	imgui_callback_list callbacks {
		main_menu_bar,
		display_viewport,
	};
	
	while (!glfwWindowShouldClose(drc.window.handle)) {
		// Refresh event list
		glfwPollEvents();
	
		// Grab the next image
		littlevk::SurfaceOperation sop;
		sop = littlevk::acquire_image(drc.device, drc.swapchain.swapchain, sync[frame.index]);
		if (sop.status == littlevk::SurfaceOperation::eResize) {
			drc.combined().resize(drc.surface, drc.window, drc.swapchain);
			rg_imgui.resize(drc);
			continue;
		}
	
		// Start the render pass
		const auto &cmd = command_buffers[frame.index];

		vk::CommandBufferBeginInfo cbbi;
		cmd.begin(cbbi);

		RenderingInfo info {
			.cmd = cmd,
			.operation = sop,
			.window = drc.window,
			.frame = frame.index,
		};

		render_viewport_scene(info, rg_viewport, viewport, vk_scene);
		render_imgui(info, rg_imgui, callbacks);

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
		if (sop.status == littlevk::SurfaceOperation::eResize) {
			drc.combined().resize(drc.surface, drc.window, drc.swapchain);
			rg_imgui.resize(drc);
		}
			
		// Toggle frame counter
		frame.index = 1 - frame.index;
	}
}