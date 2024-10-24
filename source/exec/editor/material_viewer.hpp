#pragma once

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"

using namespace jvl;
using namespace jvl::core;

// Representing values which may change over frames
// TODO: critical with timer reset...
template <typename T>
class Critical {
	T item;
	T next;
	mutable bool up_to_date = false;
public:
	Critical(const T &value, bool up_to_date_ = false)
		: next(value), up_to_date(up_to_date_) {
		if (up_to_date)
			item = next;
	}

	void queue(const T &value) {
		next = value;
		up_to_date = false;
	}

	const T &current() {
		return item;
	}

	const T &updated() {
		if (old()) {
			item = next;
			up_to_date = true;
		}

		return item;
	}

	const T *operator->() {
		return &item;
	}	

	bool old() const {
		return !up_to_date;
	}
};

struct MaterialViewer : Unique {
	Archetype <Material> ::Reference material;

	std::string main_title;

	Aperature aperature;
	Transform transform;

	// Images and framebuffer (one)
	littlevk::Image image;
	vk::Framebuffer framebuffer;
	vk::DescriptorSet descriptor;
	Critical <vk::Extent2D> extent;

	MaterialViewer(const Archetype <Material> ::Reference &);

	void display_handle(const RenderingInfo &);
	imgui_callback callback_display();
};

struct MaterialRenderGroup {
	vk::RenderPass render_pass;

	std::map <vulkan::MaterialFlags, littlevk::Pipeline> pipelines;

	MaterialRenderGroup(DeviceResourceCollection &);

	// Rendering
	void render(const RenderingInfo &, MaterialViewer &);
};