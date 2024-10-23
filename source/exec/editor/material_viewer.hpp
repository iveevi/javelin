#pragma once

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"

using namespace jvl;
using namespace jvl::core;

struct MaterialViewer : Unique {
	// TODO: ArchetypeReference instance
	const Material &material;

	std::string main_title;

	Aperature aperature;
	Transform transform;

	littlevk::Image image;

	vk::RenderPass render_pass;
	vk::Framebuffer framebuffer;

	MaterialViewer(const Material &);

	void display_handle(const RenderingInfo &);
	imgui_callback callback_display();
};