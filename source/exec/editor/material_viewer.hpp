#pragma once

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>
#include <core/adaptive_descriptor.hpp>
#include <engine/camera_controller.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"
#include "critical.hpp"

using namespace jvl;
using namespace jvl::core;

struct MaterialViewer : Unique {
	Archetype <Material> ::Reference material;

	std::string main_title;

	// View controller
	bool dragging = false;
	float theta = 0.0f;
	float phi = 0.5f * pi <float>;
	float aspect = 1.0f;
	float radius = 5.0f;

	DescriptorTable descriptors;

	// Images and framebuffer (one)
	littlevk::Image image;
	vk::Framebuffer framebuffer;
	vk::DescriptorSet viewport;
	Critical <vk::Extent2D> extent;

	MaterialViewer(const Archetype <Material> ::Reference &);

	void display_handle(const RenderingInfo &);
	imgui_callback callback_display();
};

struct MaterialRenderGroup {
	vk::RenderPass render_pass;

	wrapped::tree <vulkan::MaterialFlags, littlevk::Pipeline> pipelines;
	wrapped::tree <vk::Image, vk::DescriptorSet> solo_descriptors;

	MaterialRenderGroup(DeviceResourceCollection &);

	// Rendering
	void render(const RenderingInfo &, MaterialViewer &);
};