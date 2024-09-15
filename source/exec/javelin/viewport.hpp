#pragma once

#include <cstdint>

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>

#include "device_resource_collection.hpp"
#include "imgui_render_group.hpp"
#include "rendering_info.hpp"
#include "messaging.hpp"

using namespace jvl;
using namespace jvl::core;

// Available viewport modes
enum ViewportMode : int32_t {
	eAlbedo,
	eNormal,
	eTriangles,
	eDepth,
	eCount,
};

static constexpr const char *tbl_viewport_mode[] = {
	"Albedo",
	"Normal",
	"Triangles",
	"Depth",
	"__end",
};

static_assert(eCount + 1 == sizeof(tbl_viewport_mode)/sizeof(const char *));

// Separated from ViewportRenderGroup because we can have
// multiple viewports using the exact same render pass and pipelines
struct Viewport {
	UUID uuid;

	// Viewing information
	Aperature aperature;
	Transform transform;

	ViewportMode mode = eNormal;

	// Input handling information
	float pitch = 0;
	float yaw = 0;
	double last_x = 0;
	double last_y = 0;
	bool voided = true;
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
	imgui_callback imgui_callback();
};