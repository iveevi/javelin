#include <engine/imgui.hpp>

#include "editor.hpp"
#include "scene_inspector.hpp"

MODULE(editor);

// Removing elements of a list by global ID
template <typename T>
void remove_items(std::list <T> &list, const std::set <int64_t> &removal)
{
	auto it = list.begin();
	auto end = list.end();
	while (it != end) {
		if (removal.contains(it->uuid.global))
			it = list.erase(it);
		else
			it++;
	}
}

// Mouse and button handlers; forwarded to ImGui
void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	ImGuiIO &io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, action);
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	ImGuiIO &io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);
}

// Device extensions
static const std::vector <const char *> EXTENSIONS {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// Constructor	
Editor::Editor()
{
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

	configure_imgui(drc, rg_imgui.render_pass);

	// Other render groups		
	rg_viewport = ViewportRenderGroup(drc);

	// Configure event system(s)
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);
}

// Adding a new viewport
void Editor::add_viewport()
{
	viewports.emplace_back(drc, rg_viewport.render_pass);

	auto &viewport = viewports.back();
	imgui_callbacks.push_back(viewport.callback_display());
}

// Resizing the swapchain and related items
void Editor::resize()
{
	drc.combined().resize(drc.surface, drc.window, drc.swapchain);
	rg_imgui.resize(drc);
}

// Render loop iteration
void Editor::render(const vk::CommandBuffer &cmd, const littlevk::PresentSyncronization::Frame &sync_frame, int32_t frame)
{
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
		.drc = drc,
		.cmd = cmd,
		.operation = sop,
		.window = drc.window,
		.frame = frame,

		// Scenes
		.scene = scene,
		.device_scene = vk_scene,

		// Systems
		.message_system = message_system,
	};

	// Render all the viewports
	for (auto &viewport : viewports)
		rg_viewport.render(info, viewport);

	// Refresh any host raytracers
	for (auto &raytracer : host_raytracers)
		raytracer.refresh(drc, cmd);

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
void Editor::imgui_main_menu_bar(const RenderingInfo &)
{
	// Popup for configuring a raytracer
	bool popup_raytracer = false;

	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("View")) {
		if (ImGui::MenuItem("Viewport")) {
			add_viewport();
		}
		
		if (ImGui::MenuItem("Raytracer (CPU)")) {
			popup_raytracer = true;
		}

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	// Handle the popups
	if (popup_raytracer)
		ImGui::OpenPopup("Raytracer");
}

void Editor::imgui_raytracer_popup(const RenderingInfo &)
{
	// TODO: make stateful..
	static int samples = 1;
	static std::string preview_viewport = "Viewport (?)";
	static const Viewport *selected_viewport = nullptr;

	if (!ImGui::BeginPopup("Raytracer"))
		return;

	ImGui::Text("Viewport");

	if (ImGui::BeginCombo("##viewport", preview_viewport.c_str())) {
		for (auto &viewport : viewports) {
			std::string s = fmt::format("Viewport ({})", viewport.uuid.type_local);
			if (ImGui::Selectable(s.c_str(), selected_viewport == &viewport)) {
				preview_viewport = s;
				selected_viewport = &viewport;
			}
		}

		ImGui::EndCombo();
	}

	ImGui::Text("Samples");

	ImGui::SliderInt("##samples", &samples, 1, 16);

	ImGui::BeginDisabled(!selected_viewport);

	if (ImGui::Button("Confirm")) {
		// TODO: do on a separate thread, spawn a spinner...
		host_raytracers.emplace_back(drc,
			*selected_viewport, scene,
			vk::Extent2D(800, 800), samples);

		auto &back = host_raytracers.back();
		imgui_callbacks.emplace_back(back.callback_display());
		ImGui::CloseCurrentPopup();
	}
	
	ImGui::EndDisabled();

	ImGui::EndPopup();
}

imgui_callback Editor::callback_main_menu_bar()
{
	return {
		-1,
		std::bind(&Editor::imgui_main_menu_bar, this, std::placeholders::_1)
	};
}

imgui_callback Editor::callback_raytracer_popup()
{
	return {
		-1,
		std::bind(&Editor::imgui_raytracer_popup, this, std::placeholders::_1)
	};
}

// Rendering loop
void Editor::loop()
{
	// Synchronization information
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	// Configure for startup
	imgui_callbacks.emplace_back(callback_main_menu_bar());
	imgui_callbacks.emplace_back(callback_raytracer_popup());
	imgui_callbacks.emplace_back(inspector.scene_hierarchy_callback());
	imgui_callbacks.emplace_back(inspector.object_inspector_callback());
	
	add_viewport();

	// Rendering loop
	int32_t frame = 0;
	while (!glfwWindowShouldClose(drc.window.handle)) {
		glfwPollEvents();

		// Render the frame
		render(command_buffers[frame], sync[frame], frame);

		// Handle incoming messages
		// TODO: separate function
		auto &messages = message_system.get_origin();

		std::set <int64_t> removal_set;
		while (messages.size()) {
			auto &msg = messages.front();
			messages.pop();

			switch (msg.kind) {

			case editor_remove_self:
			{
				if (msg.type_id == type_id_of <Viewport> ()) {
					fmt::println("Viewport wants to be removed...");
					removal_set.insert(msg.global);
				} else if (msg.type_id == type_id_of <RaytracerCPU> ()) {
					fmt::println("RaytracerCPU wants to be removed...");
					removal_set.insert(msg.global);
				}
			} break;

			case editor_update_selected:
			{
				JVL_ASSERT_PLAIN(msg.type_id == type_id_of <SceneInspector> ());

				int64_t selected = msg.value.as <int64_t> ();

				// Propogate to the viewports
				for (auto &viewport : viewports)
					viewport.selected = selected;
			} break;

			default:
				break;
			}
		}

		// Removing items as requested
		// TODO: separate function
		remove_items(viewports, removal_set);
		remove_items(host_raytracers, removal_set);

		{
			// Callbacks
			auto it = imgui_callbacks.begin();
			auto end = imgui_callbacks.end();
			while (it != end) {
				if (removal_set.contains(it->global))
					it = imgui_callbacks.erase(it);
				else
					it++;
			}
		}

		// Next frame
		frame = 1 - frame;
	}

	// Cleanup
	viewports.clear();
	host_raytracers.clear();
}