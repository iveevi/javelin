#pragma once

#include <littlevk/littlevk.hpp>

#include "core/aperature.hpp"
#include "core/transform.hpp"
#include "ire/core.hpp"
#include "gfx/vk/triangle_mesh.hpp"
#include "math_types.hpp"

#include "attachment_raytracing_cpu.hpp"
#include "glio.hpp"
#include "global_context.hpp"

// Viewport information
struct AttachmentViewport {
	static constexpr const char *key = "ViewportContext";

	// Reference to 'parent'
	GlobalContext &gctx;

	// Viewing information
	jvl::core::Aperature aperature;
	jvl::core::Transform transform;

	// Vulkan structures
	vk::RenderPass render_pass;

	// Pipeline structuring
	enum : int {
		eAlbedo,
		eNormal,
		eDepth,
		eCount,
	};

	std::array <littlevk::Pipeline, eCount> pipelines;

	// Framebuffers and ImGui descriptor handle
	std::vector <littlevk::Image> targets;
	std::vector <vk::Framebuffer> framebuffers;
	std::vector <vk::DescriptorSet> imgui_descriptors;
	jvl::int2 extent;

	// Device goemetry instances
	// TODO: gfx::vulkan::Scene
	std::vector <jvl::gfx::vulkan::TriangleMesh> meshes;
	
	// Construction
	AttachmentViewport(const std::unique_ptr <GlobalContext> &);

	void configre_normal_pipeline();
	void configure_pipelines();
	void configure_render_pass();
	void configure_framebuffers();

	// Keyboard input handler
	void handle_input();

	// Rendering
	void render(const RenderState &);

	// ImGui rendering
	void render_imgui();
	
	// Rendering callback information
	Attachment attachment();
};
