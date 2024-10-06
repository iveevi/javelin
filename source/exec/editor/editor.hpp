#pragma once

#include "imgui_render_group.hpp"
#include "raytracer_cpu.hpp"
#include "rendering_info.hpp"
#include "scene_inspector.hpp"
#include "viewport.hpp"
#include "viewport_render_group.hpp"

struct Editor {
	// Fundamental Vulkan resources
	DeviceResourceCollection	drc;

	// Unique render groups
	ImGuiRenderGroup		rg_imgui;
	ViewportRenderGroup		rg_viewport;

	// Resource caches
	TextureBank			texture_bank;
	vulkan::TextureBank		device_texture_bank;

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

	// Systems
	MessageSystem			message_system;

	// Constructor
	// TODO: pass command line options later...
	Editor();

	// Adding a new viewport
	void add_viewport();

	// Resizing the swapchain and related items
	void resize();

	// Render loop iteration
	void render(const vk::CommandBuffer &, const littlevk::PresentSyncronization::Frame &, int32_t);

	// User interface methods
	void imgui_main_menu_bar(const RenderingInfo &);
	void imgui_raytracer_popup(const RenderingInfo &);

	imgui_callback callback_main_menu_bar();
	imgui_callback callback_raytracer_popup();

	// Rendering loop
	void loop();
};