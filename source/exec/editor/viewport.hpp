#pragma once

#include <cstdint>

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>
#include <engine/camera_controller.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"

using namespace jvl;
using namespace jvl::core;

// Available viewport modes
enum ViewportMode : int32_t {
	eAlbedo,
	eNormal,
	eTextureCoordinates,
	eTriangles,
	eDepth,
	eCount,
};

static constexpr const char *tbl_viewport_mode[] = {
	"Albedo",
	"Normal",
	"Texture Coordinates",
	"Triangles",
	"Depth",
	"__end",
};

static_assert(eCount + 1 == sizeof(tbl_viewport_mode)/sizeof(const char *));

// Separated from ViewportRenderGroup because we can have
// multiple viewports using the exact same render pass and pipelines
struct Viewport {
	UUID uuid;

	// Viewing angle
	Aperature aperature;
	Transform transform;
	CameraController controller;

	// Viewing information
	int64_t selected;

	ViewportMode mode = eNormal;

	// Input handling information
	bool active = false;

	// Display information
	std::string main_title;
	std::string popup_title;

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