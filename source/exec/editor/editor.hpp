#pragma once

#include "imgui_render_group.hpp"
#include "raytracer_cpu.hpp"
#include "rendering_info.hpp"
#include "scene_inspector.hpp"
#include "viewport.hpp"
#include "viewport_render_group.hpp"
#include "readable_framebuffer.hpp"
#include "material_viewer.hpp"

struct Editor {
	// Fundamental Vulkan resources
	DeviceResourceCollection drc;

	// Unique render groups
	std::unique_ptr <ImGuiRenderGroup> rg_imgui;
	std::unique_ptr <ViewportRenderGroup> rg_viewport;
	std::unique_ptr <MaterialRenderGroup> rg_material;

	// Resource caches
	TextureBank texture_bank;
	vulkan::TextureBank device_texture_bank;

	// Scene management
	core::Scene scene;
	vulkan::Scene vk_scene;

	// ImGui rendering callbacks
	imgui_callback_list imgui_callbacks;

	// Viewports
	std::list <Viewport> viewports;

	// Auxiliary framebuffers
	std::list <ReadableFramebuffer> readable_framebuffers;
	
	// Host raytracers
	std::list <RaytracerCPU> host_raytracers;

	// TODO: common interface of unique objects
	// TODO: map from type id to list of objects?
	std::list <MaterialViewer> material_viewers;
	Equivalence <Material, MaterialViewer> material_to_viewer;

	// Miscellaneous
	SceneInspector inspector;

	// Systems
	MessageSystem message_system;

	// Additional commands
	wrapped::thread_safe_queue <vk::CommandBuffer> extra;

	// Constructor
	// TODO: pass command line options later...
	Editor();

	// Adding a new viewport
	void add_viewport();

	// Resizing the swapchain and related items
	void resize();

	// Render loop iteration
	void render(const vk::CommandBuffer &, const littlevk::PresentSyncronization::Frame &, int32_t);

	// Message handling
	void process_messages();

	// User interface methods
	void imgui_main_menu_bar(const RenderingInfo &);
	void imgui_raytracer_popup(const RenderingInfo &);

	imgui_callback callback_main_menu_bar();
	imgui_callback callback_raytracer_popup();

	// Rendering loop
	void loop();
};