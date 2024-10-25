#pragma once

#include <littlevk/littlevk.hpp>

#include <core/transform.hpp>
#include <core/aperature.hpp>
#include <engine/camera_controller.hpp>

#include "imgui_render_group.hpp"
#include "rendering_info.hpp"
#include "critical.hpp"

using namespace jvl;
using namespace jvl::core;

class AdaptiveDescriptor : public vk::DescriptorSet {
	// TODO: need to add binding information as well for each key
	std::set <std::string> missing;
public:
	// Common extra keys for material-centric descriptor sets
	static constexpr const char *environment_key = "Environment";

	AdaptiveDescriptor(const vk::DescriptorSet &set = VK_NULL_HANDLE)
		: vk::DescriptorSet(set) {}

	AdaptiveDescriptor &operator=(const vk::DescriptorSet &other) {
		vk::DescriptorSet::operator=(other);
		return *this;
	}
	
	void requirement(const std::string &key) {
		missing.insert(key);
	}
	
	void resolve(const std::string &key) {
		missing.erase(key);
	}

	auto dependencies() const {
		return missing;
	}

	bool null() const {
		return static_cast <vk::DescriptorSet> (*this) == VK_NULL_HANDLE;
	}

	bool complete() const {
		return missing.empty();
	}
};

struct MaterialViewer : Unique {
	Archetype <Material> ::Reference material;

	std::string main_title;

	// View controller
	bool dragging = false;
	float theta = 0.0f;
	float phi = 0.5f * pi <float>;
	float aspect = 1.0f;
	float radius = 5.0f;

	// Mapping texture properties
	static constexpr const char *main_key = "$main";

	wrapped::tree <std::string, AdaptiveDescriptor> branches;

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

	wrapped::tree <vulkan::MaterialFlags, littlevk::Pipeline> pipelines;
	wrapped::tree <vk::Image, vk::DescriptorSet> solo_descriptors;

	MaterialRenderGroup(DeviceResourceCollection &);

	// Rendering
	void render(const RenderingInfo &, MaterialViewer &);
};