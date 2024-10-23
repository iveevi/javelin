#pragma once

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"

using namespace jvl;
using namespace jvl::core;

struct MaterialViewer : Unique {
	Archetype <Material> ::Reference material;

	std::string main_title;

	Aperature aperature;
	Transform transform;

	// Images and framebuffer (one)
	std::vector <littlevk::Image> targets;
	vk::Framebuffer framebuffer;
	vk::Extent2D extent;

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