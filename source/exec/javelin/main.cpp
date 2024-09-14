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
#include "imgui.h"
#include "ire/core.hpp"

// Local project headers
#include "device_resource_collection.hpp"
#include "littlevk/littlevk.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::ire;
using namespace jvl::gfx;

struct RenderingInfo {
	// Primary rendering structures
	const vk::CommandBuffer &cmd;
	const littlevk::SurfaceOperation &operation;
	InteractiveWindow &window;
	int32_t frame;

	// Scenes
	core::Scene &scene;
	vulkan::Scene &device_scene;
};

using imgui_callback = std::function <void (const RenderingInfo &)>;
using imgui_callback_list = std::vector <imgui_callback>;

// Renders to the swapchain images
struct ImGuiRenderGroup {
	vk::RenderPass render_pass;
	std::vector <vk::Framebuffer> framebuffers;

	// Constructors
	ImGuiRenderGroup() = default;

	ImGuiRenderGroup(DeviceResourceCollection &drc) {
		render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
			.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.done();

		resize(drc);	
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

	void render(const RenderingInfo &info, const imgui_callback_list &callbacks) {
		// Configure the rendering extent
		littlevk::viewport_and_scissor(info.cmd, littlevk::RenderArea(info.window));

		// Start the rendering pass
		auto rpbi = littlevk::default_rp_begin_info <1> (render_pass,
			framebuffers[info.operation.index], info.window);

		info.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
		
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
		
		for (auto &callback : callbacks)
			callback(info);

		// Complete the rendering	
		ImGui::Render();

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), info.cmd);
		
		// Finish the rendering pass
		info.cmd.endRenderPass();
	}
};

enum ViewportMode : int32_t {
	eAlbedo,
	eNormal,
	eDepth,
	eCount,
};

// Separated from ViewportRenderGroup because we can have
// multiple viewports using the exact same render pass and pipelines
struct Viewport {
	int64_t uuid;

	// Viewing information
	jvl::core::Aperature aperature;
	jvl::core::Transform transform;

	ViewportMode mode = eNormal;

	// Input handling information
	float pitch = 0;
	float yaw = 0;
	bool active = false;

	// Images and framebuffers
	std::vector <littlevk::Image> targets;
	std::vector <vk::Framebuffer> framebuffers;
	std::vector <vk::DescriptorSet> imgui_descriptors;
	vk::Extent2D extent;

	Viewport(DeviceResourceCollection &drc,
		       const vk::RenderPass &render_pass,
		       int64_t uuid_) : uuid(uuid_) {
		extent = drc.window.extent;
		resize(drc, render_pass);
	}
	
	void handle_input(const InteractiveWindow &window) {
		static constexpr float speed = 100.0f;
		static float last_time = 0.0f;

		if (!active)
			return;
		
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

	void cursor_handle(const WindowEventSystem::cursor_event &event) {
		std::string title = fmt::format("Viewport##{}", uuid);
		ImGui::SetWindowFocus(title.c_str());

		if (!event.drag)
			return;

		static constexpr float sensitivity = 0.0025f;
		
		pitch -= event.dx * sensitivity;
		yaw += event.dy * sensitivity;
		
		float pi_e = pi <float> / 2.0f - 1e-3f;
		yaw = std::min(pi_e, std::max(-pi_e, yaw));

		transform.rotation = fquat::euler_angles(yaw, pitch, 0);
	}

	void display_handle(const RenderingInfo &info) {
		bool open = true;
		std::string title = fmt::format("Viewport##{}", uuid);
		ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_MenuBar);

		active = ImGui::IsWindowFocused();

		if (ImGui::BeginMenuBar()) {
			if (ImGui::MenuItem("Render")) {
				fmt::println("triggering cpu raytracer...");
			}

			ImGui::EndMenuBar();
		}

		ImVec2 size;
		size = ImGui::GetContentRegionAvail();
		ImGui::Image(imgui_descriptors[info.frame], size);

		size = ImGui::GetItemRectSize();
		aperature.aspect = size.x/size.y;

		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();

		auto &region = info.window.event_system.regions[uuid];
		region.set_bounds(min, max);

		ImGui::GetForegroundDrawList()->AddRect(min, max, IM_COL32(255, 0, 0, 255));

		ImGui::End();
	}

	auto cursor_callback() {
		return std::bind(&Viewport::cursor_handle, this, std::placeholders::_1);
	}

	auto imgui_callback() {
		return std::bind(&Viewport::display_handle, this, std::placeholders::_1);
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

	// Constructors
	ViewportRenderGroup() = default;

	ViewportRenderGroup(DeviceResourceCollection &drc) {
		configure_render_pass(drc);
		configure_pipelines(drc);
	}

	auto render_pass_begin_info(const RenderingInfo &info, const Viewport &viewport) const {
		return littlevk::default_rp_begin_info <2> (render_pass,
			viewport.framebuffers[info.operation.index],
			viewport.extent);
	}

	void render(const RenderingInfo &info, Viewport &viewport) {
		// Configure the rendering extent
		littlevk::viewport_and_scissor(info.cmd, littlevk::RenderArea(viewport.extent));

		viewport.handle_input(info.window);

		auto rpbi = littlevk::default_rp_begin_info <2> (render_pass,
			viewport.framebuffers[info.operation.index],
			viewport.extent);

		info.cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

		auto &ppl = pipelines[viewport.mode];

		info.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

		solid_t <MVP> mvp;

		auto m_model = uniform_field(MVP, model);
		auto m_view = uniform_field(MVP, view);
		auto m_proj = uniform_field(MVP, proj);

		mvp[m_model] = jvl::float4x4::identity();
		mvp[m_proj] = jvl::core::perspective(viewport.aperature);
		mvp[m_view] = viewport.transform.to_view_matrix();

		info.cmd.pushConstants <solid_t <MVP>> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);

		for (const auto &vmesh : info.device_scene.meshes) {
			info.cmd.bindVertexBuffers(0, vmesh.vertices.buffer, { 0 });
			info.cmd.bindIndexBuffer(vmesh.triangles.buffer, 0, vk::IndexType::eUint32);
			info.cmd.drawIndexed(vmesh.count, 1, 0, 0, 0);
		}

		info.cmd.endRenderPass();

		littlevk::transition(info.cmd, viewport.targets[info.operation.index],
			vk::ImageLayout::ePresentSrcKHR,
			vk::ImageLayout::eShaderReadOnlyOptimal);
	}
};

struct SceneInspector {
	// TODO: uuid lookup
	const Scene::Object *selected = nullptr;

	void scene_hierarchy_object(const Scene::Object &obj) {
		if (obj.children.empty()) {
			if (ImGui::Selectable(obj.name.c_str(), selected == &obj))
				selected = &obj;
		} else {
			ImGuiTreeNodeFlags flags;
			flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (selected == &obj)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool opened = ImGui::TreeNodeEx(obj.name.c_str(), flags);
			if (ImGui::IsItemClicked())
				selected = &obj;

			if (opened) {
				for (const auto &ref : obj.children)
					scene_hierarchy_object(*ref);

				ImGui::TreePop();
			}
		}
	}

	void scene_hierarchy(const RenderingInfo &info) {
		ImGui::Begin("Scene");

		for (auto &root_ptr: info.scene.root) {
			auto &obj = *root_ptr;
			scene_hierarchy_object(obj);
		}

		ImGui::End();
	}

	void object_inspector(const RenderingInfo &info) {
		ImGui::Begin("Inspector");

		if (selected) {
			auto &obj = *selected;
			ImGui::Text("%s", obj.name.c_str());

			if (ImGui::CollapsingHeader("Mesh")) {
				if (obj.geometry) {
					auto &g = obj.geometry.value();
					// TODO: separate method to list all properties
					ImGui::Text("# of vertices: %lu", g.vertex_count);

					auto &triangles = g.face_properties.at(Mesh::triangle_key);
					ImGui::Text("# of triangles: %lu", typed_buffer_size(triangles));
				}
			}
			
			if (ImGui::CollapsingHeader("Materials")) {
				ImGui::Text("Count: %lu", obj.materials.size());
			}
		}

		ImGui::End();
	}

	auto scene_hierarchy_callback() {
		return std::bind(&SceneInspector::scene_hierarchy, this, std::placeholders::_1);
	}

	auto object_inspector_callback() {
		return std::bind(&SceneInspector::object_inspector, this, std::placeholders::_1);
	}
};

// Device extensions
static const std::vector <const char *> EXTENSIONS {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct Editor {
	// Fundamental Vulkan resources
	DeviceResourceCollection	drc;

	// Unique render groups
	ImGuiRenderGroup		rg_imgui;
	ViewportRenderGroup		rg_viewport;

	// Scene management
	core::Scene			scene;
	vulkan::Scene			vk_scene;

	// ImGui rendering callbacks
	imgui_callback_list		imgui_callbacks;

	// Supporting multiple viewports
	std::list <Viewport>		viewports;

	// Miscellaneous
	SceneInspector			inspector;

	// Constructor
	// TODO: pass command line options later...
	Editor() {
		// Load physical device
		auto predicate = [](vk::PhysicalDevice phdev) {
			return littlevk::physical_device_able(phdev, EXTENSIONS);
		};

		// Configure the resource collection
		DeviceResourceCollectionInfo info {
			.phdev = littlevk::pick_physical_device(predicate),
			.title = "Editor",
			.extent = vk::Extent2D(1920, 1080),
			.extensions = EXTENSIONS,
		};
		
		drc = DeviceResourceCollection::from(info);

		// ImGui configuration
		rg_imgui = ImGuiRenderGroup(drc);

		imgui::configure_vulkan(drc, rg_imgui.render_pass);

		// Other render groups		
		rg_viewport = ViewportRenderGroup(drc);

		// Configure event system(s)
		glfwSetWindowUserPointer(drc.window.handle, &drc.window.event_system);
		glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
		glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);
	}

	// Adding a new viewport
	void add_viewport() {
		int64_t viewport_uuid = drc.window.event_system.new_uuid();
		viewports.emplace_back(drc, rg_viewport.render_pass, viewport_uuid);

		auto &viewport = viewports.back();
		drc.window.event_system.regions[viewport_uuid]
			.set_cursor_callback(viewport.cursor_callback());
		
		imgui_callbacks.push_back(viewport.imgui_callback());
	}

	// Resizing the swapchain and related items
	void resize() {
		drc.combined().resize(drc.surface, drc.window, drc.swapchain);
		rg_imgui.resize(drc);
	}

	// Render loop iteration
	void render(const vk::CommandBuffer &cmd, const littlevk::PresentSyncronization::Frame &sync_frame, int32_t frame) {
		// Grab the next image
		littlevk::SurfaceOperation sop;
		sop = littlevk::acquire_image(drc.device, drc.swapchain.swapchain, sync_frame);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			return resize();
	
		// Start the render pass
		vk::CommandBufferBeginInfo cbbi;
		cmd.begin(cbbi);

		RenderingInfo info {
			// Rendering structures
			.cmd = cmd,
			.operation = sop,
			.window = drc.window,
			.frame = frame,

			// Device scenes
			.scene = scene,
			.device_scene = vk_scene
		};

		// Render all the viewports
		for (auto &viewport : viewports)
			rg_viewport.render(info, viewport);

		// Finally, render the user interface
		auto current_callbacks = imgui_callbacks;
		rg_imgui.render(info, current_callbacks);

		cmd.end();
	
		// Submit command buffer while signaling the semaphore
		constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		vk::SubmitInfo submit_info {
			sync_frame.image_available,
			wait_stage, cmd,
			sync_frame.render_finished
		};

		drc.graphics_queue.submit(submit_info, sync_frame.in_flight);

		// Present to the window
		sop = littlevk::present_image(drc.present_queue, drc.swapchain.swapchain, sync_frame, sop.index);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			resize();
	}

	// Main menu bar
	void imgui_main_menu_bar(const RenderingInfo &) {
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Viewport")) {
				add_viewport();
			}
			
			if (ImGui::MenuItem("Raytracer (CPU)")) {

			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	auto main_menu_bar_callback() {
		return std::bind(&Editor::imgui_main_menu_bar, this, std::placeholders::_1);
	}

	// Rendering loop
	void loop() {
		// Synchronization information
		auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

		// Command buffers for the rendering loop
		auto command_buffers = littlevk::command_buffers(drc.device,
			drc.command_pool,
			vk::CommandBufferLevel::ePrimary, 2u);

		// Configure for startup
		imgui_callbacks.emplace_back(main_menu_bar_callback());
		imgui_callbacks.emplace_back(inspector.scene_hierarchy_callback());
		imgui_callbacks.emplace_back(inspector.object_inspector_callback());
		
		add_viewport();

		// Rendering loop
		int32_t frame = 0;
		while (!glfwWindowShouldClose(drc.window.handle)) {
			glfwPollEvents();
			render(command_buffers[frame], sync[frame], frame);
			frame = 1 - frame;
		}
	}
};

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	auto &config = littlevk::config();
	config.enable_logging = false;
	config.abort_on_validation_error = true;

	Editor editor;

	// Load the asset and scene
	std::filesystem::path path = argv[1];

	auto asset = engine::ImportedAsset::from(path).value();
	editor.scene = core::Scene();
	editor.scene.add(asset);

	// Prepare host and device scenes
	auto host_scene = cpu::Scene::from(editor.scene);
	editor.vk_scene = vulkan::Scene::from(editor.drc.allocator(), host_scene);

	// Main loop
	editor.loop();
}