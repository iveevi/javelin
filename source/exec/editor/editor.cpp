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

template <typename T>
void remove_items(std::list <T> &list, const std::set <int64_t> &removal, const auto &action)
{
	auto it = list.begin();
	auto end = list.end();
	while (it != end) {
		if (removal.contains(it->uuid.global)) {
			action(*it);
			it = list.erase(it);
		} else {
			it++;
		}
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
	rg_imgui = std::make_unique <ImGuiRenderGroup> (drc);

	configure_imgui(drc, rg_imgui->render_pass);

	// Other render groups		
	rg_viewport = std::make_unique <ViewportRenderGroup> (drc);
	rg_material = std::make_unique <MaterialRenderGroup> (drc);

	// Configure event system(s)
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);

	// Populate boilerplate textures
	auto checkerboard = core::Texture::from("resources/textures/checkerboard.png");
	texture_bank["$checkerboard"] = checkerboard;
	device_texture_bank.upload(drc.allocator(),
		drc.commander(),
		"$checkerboard",
		checkerboard);
}

// Adding a new viewport
void Editor::add_viewport()
{
	viewports.emplace_back(drc, rg_viewport->render_pass);

	auto &viewport = viewports.back();
	imgui_callbacks.push_back(viewport.callback_display());
}

// Resizing the swapchain and related items
void Editor::resize()
{
	drc.combined().resize(drc.surface, drc.window, drc.swapchain);
	rg_imgui->resize(drc);
}

// Render loop iteration
void Editor::render(const vk::CommandBuffer &cmd, const littlevk::PresentSyncronization::Frame &sync_frame, int32_t frame)
{
	// Grab the next image
	littlevk::SurfaceOperation sop;
	sop = littlevk::acquire_image(drc.device, drc.swapchain.handle, sync_frame);
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

		// Caches
		.texture_bank = texture_bank,
		.device_texture_bank = device_texture_bank,

		// Scenes
		.scene = scene,
		.device_scene = vk_scene,

		// Systems
		.message_system = message_system,

		// Additional command buffers
		.extra = extra,
	};

	// Render all the viewports
	for (auto &viewport : viewports)
		rg_viewport->render(info, viewport);

	// Render any material viewers
	for (auto &viewer : material_viewers)
		rg_material->render(info, viewer);

	// Refresh any host raytracers
	for (auto &raytracer : host_raytracers)
		raytracer.refresh(drc, cmd);

	// Finally, render the user interface
	auto current_callbacks = imgui_callbacks;
	rg_imgui->render(info, current_callbacks);

	// Process any readable framebuffers
	for (auto &rf : readable_framebuffers)
		rf.go(info);

	cmd.end();

	// Submit command buffer while signaling the semaphore
	constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	std::vector <vk::CommandBuffer> commands { cmd };
	while (extra.size()) {
		auto cmd = extra.pop();
		commands.push_back(cmd);
	}

	vk::SubmitInfo submit_info {
		sync_frame.image_available,
		wait_stage, commands,
		sync_frame.render_finished
	};

	drc.graphics_queue.submit(submit_info, sync_frame.in_flight);

	// Present to the window
	sop = littlevk::present_image(drc.present_queue, drc.swapchain.handle, sync_frame, sop.index);
	if (sop.status == littlevk::SurfaceOperation::eResize)
		resize();

	// Post rendering actions
	rg_viewport->post_render();
}

// Main menu bar
void Editor::imgui_main_menu_bar(const RenderingInfo &)
{
	// Popup for configuring a raytracer
	bool popup_raytracer = false;
	bool popup_pipelines = false;

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

	if (ImGui::BeginMenu("Debug")) {
		if (ImGui::MenuItem("Render Pipelines")) {
			popup_pipelines = true;
		}

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	// Handle the popups
	if (popup_raytracer)
		ImGui::OpenPopup("Raytracer");
	
	if (popup_pipelines)
		ImGui::OpenPopup("Render Pipelines");
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

// Processing messages
void Editor::process_messages()
{
	// Handle incoming messages
	auto &messages = message_system.get_origin();

	std::set <int64_t> removal_set;
	while (messages.size()) {
		auto &msg = messages.front();
		messages.pop();

		switch (msg.kind) {

		case editor_remove_self:
		{
			static const std::set <int64_t> quick_fetch {
				type_id_of <Viewport> (),
				type_id_of <RaytracerCPU> (),
				type_id_of <ReadableFramebuffer> (),
				type_id_of <MaterialViewer> (),
			};

			if (quick_fetch.contains(msg.type_id))
				removal_set.insert(msg.global);
			else
				JVL_ABORT("removal requested for unrecognized type UUID={}", msg.type_id);
		} break;

		case editor_viewport_update_selected:
		{
			JVL_ASSERT_PLAIN(msg.type_id == type_id_of <SceneInspector> ());

			int64_t selected = msg.value.as <int64_t> ();

			// Propogate to the viewports
			for (auto &viewport : viewports)
				viewport.selected = selected;
		} break;

		case editor_viewport_selection:
		{
			// Find the corresponding viewport
			auto it = std::find_if(viewports.begin(), viewports.end(),
				[&](const Viewport &viewport) {
					return msg.global == viewport.id();
				});

			fmt::println("creating a readable framebuffer for rendering object ids: {} x {}",
					it->extent.width, it->extent.height);

			ReadableFramebuffer::ObjectSelection request;
			request.pixel = msg.value.as <int2> ();
			
			readable_framebuffers.emplace_back(drc, *it, vk::Format::eR32Sint);
			readable_framebuffers.back().apply_request(request);
		} break;

		case editor_update_selected_object:
		{
			int id = msg.value.as <int64_t> ();
			fmt::println("updating to id: {}", id);

			// TODO: method
			inspector.selected = id;
			for (auto &viewport : viewports)
				viewport.selected = id;
		} break;

		case editor_open_material_inspector:
		{
			int64_t id = msg.value.as <int64_t> ();
			// Skip if already instantiated
			if (material_to_viewer.has_a(id)) {
				fmt::println("viewer already open... UUID={}",
					material_to_viewer.a_to_b[id]);
			} else {
				fmt::println("opening new material inspector for material with id: {}", id);

				int64_t index = msg.value.as <int64_t> ();
				auto ref = scene.materials[index];
				material_viewers.emplace_back(ref);
				
				auto &viewer = material_viewers.back();
				imgui_callbacks.push_back(viewer.callback_display());

				material_to_viewer.add(id, viewer);
			}
		} break;

		default:
			break;
		}
	}

	// Removing items as requested
	auto remove_material_viewer = [&](const MaterialViewer &viewer) {
		fmt::println("removing material viewer: UUID={}", viewer.id());
		material_to_viewer.remove_b(viewer);
	};

	remove_items(viewports, removal_set);
	remove_items(host_raytracers, removal_set);
	remove_items(readable_framebuffers, removal_set);
	remove_items(material_viewers, removal_set, remove_material_viewer);

	{
		// Callbacks
		// TODO: derive from unique
		auto it = imgui_callbacks.begin();
		auto end = imgui_callbacks.end();
		while (it != end) {
			if (removal_set.contains(it->global))
				it = imgui_callbacks.erase(it);
			else
				it++;
		}
	}
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
	imgui_callbacks.emplace_back(inspector.callback_scene_hierarchy());
	imgui_callbacks.emplace_back(inspector.callback_object_inspector());
	imgui_callbacks.emplace_back(rg_viewport->callback_debug_pipelines());
	
	add_viewport();

	// Rendering loop
	int32_t frame = 0;
	while (!glfwWindowShouldClose(drc.window.handle)) {
		glfwPollEvents();

		// Render the frame
		render(command_buffers[frame], sync[frame], frame);

		// Handle incoming messages
		process_messages();

		// Next frame
		frame = 1 - frame;
	}

	// Cleanup
	viewports.clear();
	host_raytracers.clear();
}