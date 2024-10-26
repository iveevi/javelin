#pragma once

#include "imgui_render_group.hpp"
#include "raytracer_cpu.hpp"
#include "rendering_info.hpp"
#include "scene_inspector.hpp"
#include "viewport.hpp"
#include "viewport_render_group.hpp"
#include "readable_framebuffer.hpp"
#include "material_viewer.hpp"
#include "texture_loading.hpp"

struct Timer {
	using clock_t = std::chrono::high_resolution_clock;

	clock_t clock;
	clock_t::time_point last;

	void begin() {
		last = clock.now();
	}

	[[nodiscard]]
	double end() {
		auto now = clock.now();
		auto duration = std::chrono::duration_cast <std::chrono::microseconds> (now - last).count();
		return (duration / 1000.0);
	}
};

using TimeTable = wrapped::tree <std::string, double>;

struct ScopedTimer {
	Timer timer;
	TimeTable &ref;
	std::string label;

	ScopedTimer(TimeTable &ref_, const std::string &label_)
			: ref(ref_), label(label_) {
		timer.begin();
	}

	~ScopedTimer() {
		ref[label] = timer.end();
	}
};

struct Editor {
	// Fundamental Vulkan resources
	DeviceResourceCollection drc;

	// Unique render groups
	std::unique_ptr <ImGuiRenderGroup> renderer_imgui;
	std::unique_ptr <ViewportRenderGroup> renderer_viewport;
	std::unique_ptr <MaterialRenderGroup> renderer_material;

	// Thread workers
	std::unique_ptr <TextureLoadingWorker> worker_texture_loading;

	// Resource caches
	TextureBank host_bank;
	vulkan::TextureBank device_bank;

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

	// Timing resources
	bool display_times;
	TimeTable previous_times;

	// Miscellaneous
	SceneInspector inspector;

	// Systems
	MessageSystem message_system;

	// Managing texture transitions
	wrapped::thread_safe_queue <TextureTransitionUnit> transitions;
	wrapped::tree <vk::Fence, std::set <std::string>> transitions_progress;

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
	void imgui_times_popup(const RenderingInfo &);

	imgui_callback callback_main_menu_bar();
	imgui_callback callback_popup_raytracer();
	imgui_callback callback_popup_times();

	// Rendering loop
	void loop();
};