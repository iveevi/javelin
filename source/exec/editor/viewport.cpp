#include <imgui/imgui_internal.h>

#include <engine/imgui.hpp>

#include "viewport.hpp"

Viewport::Viewport(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
		: Unique(new_uuid <Viewport> ()), controller(transform, engine::CameraControllerSettings())
{
	extent = drc.window.extent;
	resize(drc, render_pass);

	main_title = fmt::format("Viewport ({})##{}", uuid.type_local, uuid.global);
	popup_camera_settings_title= fmt::format("Camera Settings##{}", uuid.global);
	popup_view_mode_title = fmt::format("View Mode##{}", uuid.global);

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
	std::tie(image, depth) = drc.allocator()
		.image(extent,
			vk::Format::eB8G8R8A8Unorm,
			vk::ImageUsageFlagBits::eColorAttachment
			| vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eColor)
		.image(extent,
			vk::Format::eD32Sfloat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth);

	std::array <vk::ImageView, 2> views {
		image.view,
		depth.view,
	};

	auto framebuffer_info = vk::FramebufferCreateInfo()
		.setAttachments(views)
		.setLayers(1)
		.setWidth(extent.width)
		.setHeight(extent.height)
		.setRenderPass(render_pass);

	framebuffer = drc.device.createFramebuffer(framebuffer_info);

	vk::Sampler sampler = littlevk::SamplerAssembler(drc.device, drc.dal);

	descriptor = imgui_texture_descriptor(sampler,
		image.view,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	auto transition = [&](const vk::CommandBuffer &cmd) {
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

	bool popup_camera_settings = false;
	bool popup_view_mode = false;

	if (ImGui::BeginMenuBar()) {
		if (ImGui::MenuItem("Display"))
			popup_view_mode = true;
		
		if (ImGui::MenuItem("Camera"))
			popup_camera_settings = true;

		if (ImGui::MenuItem("Render"))
			fmt::println("triggering cpu raytracer...");

		ImGui::EndMenuBar();
	}

	ImVec2 size = ImGui::GetContentRegionAvail();
	aperature.aspect = size.x / size.y;

	ImGui::ImageButton(descriptor, size, ImVec2(0, 0), ImVec2(1, 1), 0);
	ImGui::SetItemUsingMouseWheel();

	if (ImGui::IsItemActive()) {
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
			
			if (!controller.dragging) {
				glfwSetInputMode(info.drc.window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				controller.dragging = true;
			}

			controller.handle_delta(float2(delta.x, delta.y));

			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
		} else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			ImVec2 mouse = ImGui::GetMousePos();
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();

			float2 relative {
				(mouse.x - min.x)/(max.x - min.x),
				(mouse.y - min.y)/(max.y - min.y)
			};

			int2 pixel {
				int(extent.width * relative.x),
				int(extent.height * relative.y)
			};

			auto selection_message = message(editor_viewport_selection, pixel);
			info.message_system.send_to_origin(selection_message);
		}
	} else {
		if (controller.dragging) {
			glfwSetInputMode(info.drc.window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			controller.dragging = false;
		}
	}

	ImGui::End();

	if (popup_view_mode)
		ImGui::OpenPopup(popup_view_mode_title.c_str());
	
	if (popup_camera_settings)
		ImGui::OpenPopup(popup_camera_settings_title.c_str());

	if (ImGui::BeginPopup(popup_view_mode_title.c_str())) {
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
	
	if (ImGui::BeginPopup(popup_camera_settings_title.c_str())) {
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