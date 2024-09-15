#include "viewport.hpp"

Viewport::Viewport(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
		: uuid(new_uuid <Viewport> ())
{
	extent = drc.window.extent;
	resize(drc, render_pass);

	main_title = fmt::format("Viewport ({})##{}", uuid.type_local, uuid.global);
	popup_title = fmt::format("Mode##{}", uuid.global);
}

void Viewport::handle_input(const InteractiveWindow &window)
{
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

void Viewport::display_handle(const RenderingInfo &info)
{
	bool open = true;
	
	ImGui::Begin(main_title.c_str(), &open, ImGuiWindowFlags_MenuBar);
	if (!open) {
		Message message {
			.type_id = uuid.type_id,
			.global = uuid.global,
			.kind = eRemoveSelf,
		};

		info.message_system.send_to_origin(message);
	}

	active = ImGui::IsWindowFocused();

	bool popup = false;

	if (ImGui::BeginMenuBar()) {
		if (ImGui::MenuItem("Display")) {
			popup = true;
		}

		if (ImGui::MenuItem("Render")) {
			fmt::println("triggering cpu raytracer...");
		}

		ImGui::EndMenuBar();
	}

	ImVec2 size;
	size = ImGui::GetContentRegionAvail();
	ImGui::ImageButton(imgui_descriptors[info.frame], size, ImVec2(0, 0), ImVec2(1, 1), 0);

	size = ImGui::GetItemRectSize();
	aperature.aspect = size.x/size.y;

	// Input handling
	if (ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		ImVec2 mouse = ImGui::GetMousePos();

		static constexpr float sensitivity = 0.0025f;

		if (voided) {
			last_x = mouse.x;
			last_y = mouse.y;
			voided = false;
		}

		double dx = mouse.x - last_x;
		double dy = mouse.y - last_y;

		// Dragging state
		pitch -= dx * sensitivity;
		yaw += dy * sensitivity;

		float pi_e = pi <float> / 2.0f - 1e-3f;
		yaw = std::min(pi_e, std::max(-pi_e, yaw));

		transform.rotation = fquat::euler_angles(yaw, pitch, 0);

		last_x = mouse.x;
		last_y = mouse.y;
	} else {
		voided = true;
	}

	ImGui::End();

	if (popup)
		ImGui::OpenPopup(popup_title.c_str());

	if (ImGui::BeginPopup(popup_title.c_str())) {
		bool selected = false;
		for (int32_t i = 0; i < eCount; i++) {
			if (ImGui::Selectable(tbl_viewport_mode[i])) {
				mode = (ViewportMode) i;
				selected = true;
				break;
			}
		}

		if (selected)
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}

imgui_callback Viewport::imgui_callback()
{
	return {
		uuid.global,
		std::bind(&Viewport::display_handle, this, std::placeholders::_1)
	};
}