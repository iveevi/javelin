#pragma once

#include <cstdint>

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>
#include <engine/camera_controller.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"
#include "render_mode.hpp"

using namespace jvl;
using namespace jvl::core;

// Separated from ViewportRenderGroup because we can have
// multiple viewports using the exact same render pass and pipelines
struct Viewport : Unique {
	// Viewing angle
	Aperature aperature;
	Transform transform;
	CameraController controller;

	// Viewing information
	int64_t selected;

	RenderMode mode = RenderMode::eNormalSmooth;

	// Input handling information
	bool active = false;

	// Display information
	std::string main_title;
	std::string camera_settings_popup_title;
	std::string view_mode_popup_title;

	// Images and framebuffers
	std::vector <littlevk::Image> targets;
	std::vector <vk::Framebuffer> framebuffers;
	std::vector <vk::DescriptorSet> imgui_descriptors;
	vk::Extent2D extent;

	Viewport(DeviceResourceCollection &, const vk::RenderPass &);

	void handle_input(const InteractiveWindow &);
	void resize(DeviceResourceCollection &, const vk::RenderPass &);
	void display_handle(const RenderingInfo &);
	imgui_callback callback_display();
};