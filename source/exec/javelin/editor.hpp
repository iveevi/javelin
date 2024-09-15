#pragma once

#include "device_resource_collection.hpp"
#include "imgui_render_group.hpp"
#include "viewport_render_group.hpp"
#include "viewport.hpp"
#include "raytracer_cpu.hpp"
#include "scene_inspector.hpp"

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

	// Viewports
	std::list <Viewport>		viewports;
	
	// Host raytracers
	std::list <RaytracerCPU>	host_raytracers;

	// Miscellaneous
	SceneInspector			inspector;

	// Constructor
	// TODO: pass command line options later...
	Editor();

	// Adding a new viewport
	void add_viewport();

	// Resizing the swapchain and related items
	void resize();

	// Render loop iteration
	void render(const vk::CommandBuffer &, const littlevk::PresentSyncronization::Frame &, int32_t);

	// Main menu bar
	void imgui_main_menu_bar(const RenderingInfo &);
	imgui_callback main_menu_bar_callback();

	// Rendering loop
	void loop();
};