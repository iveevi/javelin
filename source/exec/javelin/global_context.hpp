#pragma once

#include <littlevk/littlevk.hpp>

#include "wrapped_types.hpp"
#include "core/scene.hpp"

#include "device_resource_collection.hpp"

// Prelude information for the global context
struct GlobalContextPrelude {
	vk::PhysicalDevice phdev;
	vk::Extent2D extent;
	std::string title;
	std::vector <const char *> extensions;
};

// Render loop information for other contexts
struct RenderState {
	vk::CommandBuffer cmd;
	littlevk::SurfaceOperation sop;
	int32_t frame;
};

// Attachments to the global context for the main rendering loop
struct Attachment {
	using render_impl = std::function <void (const RenderState &)>;
	using resize_impl = std::function <void ()>;

	render_impl renderer;
	resize_impl resizer;
	void *user;
};

// Global state of the engine backend
struct GlobalContext {
	// GPU (active) resources
	DeviceResourceCollection drc;

	// Active scene
	jvl::core::Scene scene;

	// Attachments from other contexts
	// TODO: attachment dependencies
	jvl::wrapped::hash_table <std::string, Attachment> attachments;
	// std::map <std::string, Attachment> attachments;

	// Rendering
	struct {
		int32_t index = 0;
	} frame;

	// Construction	
	GlobalContext(const GlobalContextPrelude &);

	// Attachment management
	void attach(const std::string &, const Attachment &);

	template <typename T>
	T *attachment_user() {
		return reinterpret_cast <T *> (attachments[T::key].user);
	}

	// Resizing
	void resize();

	// Primary rendering loop
	void loop();
};