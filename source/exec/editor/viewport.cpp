#include <engine/imgui.hpp>

#include "viewport.hpp"
#include "imgui.h"

Viewport::Viewport(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
		: Unique(new_uuid <Viewport> ()), controller(transform, engine::CameraControllerSettings())
{
	extent = drc.window.extent;
	resize(drc, render_pass);

	main_title = fmt::format("Viewport ({})##{}", uuid.type_local, uuid.global);
	camera_settings_popup_title= fmt::format("Camera Settings##{}", uuid.global);
	view_mode_popup_title = fmt::format("View Mode##{}", uuid.global);

	// Presets for the camera controller
	controller.settings.up = GLFW_KEY_E;
	controller.settings.down  = GLFW_KEY_Q;
	controller.settings.invert_y = true;
}

void Viewport::handle_input(const InteractiveWindow &window)
{
	if (!active)
		return;

	controller.handle_movement(window);
}

void Viewport::resize(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
{
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
		vk::DescriptorSet dset = imgui_texture_descriptor(sampler, image.view,
			vk::ImageLayout::eShaderReadOnlyOptimal);

		imgui_descriptors.push_back(dset);
	}

	auto transition = [&](const vk::CommandBuffer &cmd) {
		for (auto &image : targets)
			image.transition(cmd, vk::ImageLayout::ePresentSrcKHR);
	};

	drc.commander().submit_and_wait(transition);
}

void Viewport::display_handle(const RenderingInfo &info)
{
	bool open = true;
	
	ImGui::Begin(main_title.c_str(), &open, ImGuiWindowFlags_MenuBar);
	if (!open) {
		auto remove_request = message(editor_remove_self);
		info.message_system.send_to_origin(remove_request);
	}

	active = ImGui::IsWindowFocused();

	bool camera_settings_popup = false;
	bool view_mode_popup = false;

	if (ImGui::BeginMenuBar()) {
		if (ImGui::MenuItem("Display"))
			view_mode_popup = true;
		
		if (ImGui::MenuItem("Camera"))
			camera_settings_popup = true;

		if (ImGui::MenuItem("Render"))
			fmt::println("triggering cpu raytracer...");

		ImGui::EndMenuBar();
	}

	ImVec2 size;

	size = ImGui::GetContentRegionAvail();
	ImGui::ImageButton(imgui_descriptors[info.frame], size, ImVec2(0, 0), ImVec2(1, 1), 0);

	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		ImVec2 mouse = ImGui::GetMousePos();
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();

		float2 relative {
			(mouse.x - min.x)/(max.x - min.x),
			(mouse.y - min.y)/(max.y - min.y)
		};

		if (relative.x >= 0 && relative.x <= 1
			&& relative.y >= 0 && relative.y <= 1) {
			fmt::println("double click!");
			fmt::println("min/max = ({}, {})/({}, {})", min.x, min.y, max.x, max.y);
			fmt::println("  relative position: {}, {}", relative.x, relative.y);
			int2 pixel {
				int(extent.width * relative.x),
				int(extent.height * relative.y)
			};
			fmt::println("  pixel position: {} {}", pixel.x, pixel.y);

			auto selection_message = message(editor_viewport_selection, pixel);
			info.message_system.send_to_origin(selection_message);
		}
	}

	size = ImGui::GetItemRectSize();
	aperature.aspect = size.x/size.y;

	// Input handling
	if (ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		ImVec2 mouse = ImGui::GetMousePos();

		bool previous = controller.dragging;
		controller.dragging = true;
		controller.handle_cursor(float2(mouse.x, mouse.y));
		
		if (!previous) {
			fmt::println("initial dragging");
			glfwSetInputMode(info.drc.window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	} else {
		if (controller.dragging) {
			fmt::println("finished dragging");
			glfwSetInputMode(info.drc.window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		
		controller.voided = true;
		controller.dragging = false;
	}

	ImGui::End();

	if (view_mode_popup)
		ImGui::OpenPopup(view_mode_popup_title.c_str());
	
	if (camera_settings_popup)
		ImGui::OpenPopup(camera_settings_popup_title.c_str());

	if (ImGui::BeginPopup(view_mode_popup_title.c_str())) {
		bool selected = false;
		for (int32_t i = 0; i < int32_t(RenderMode::eCount); i++) {
			if (i == uint32_t(RenderMode::eBackup))
				continue;

			if (ImGui::Selectable(tbl_render_mode[i])) {
				mode = (RenderMode) i;
				selected = true;
				break;
			}
		}

		if (selected)
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
	
	if (ImGui::BeginPopup(camera_settings_popup_title.c_str())) {
		ImGui::SliderFloat("speed", &controller.settings.speed, 1, 1000);
		ImGui::SliderFloat("sensitivity", &controller.settings.sensitivity, 1, 10);
		ImGui::EndPopup();
	}
}

imgui_callback Viewport::callback_display()
{
	return {
		uuid.global,
		std::bind(&Viewport::display_handle, this, std::placeholders::_1)
	};
}