#pragma once

#include "global_context.hpp"

// User interface information
struct AttachmentUI {
	// Reference to 'parent'
	GlobalContext &gctx;

	// Vulkan structures
	vk::RenderPass render_pass;

	// Framebuffers
	std::vector <vk::Framebuffer> framebuffers;
	
	// Drawing callbacks
	std::vector <std::function <void ()>> callbacks;

	// Construction
	AttachmentUI(const std::unique_ptr <GlobalContext> &);

	void configure_render_pass();
	void configure_framebuffers();

	// Rendering
	void render(const RenderState &);

	// Rendering callback information
	Attachment attachment();

	// Adding drawing callbacks
	template <typename ... Args>
	void subscribe(Args ... args) {
		callbacks.push_back(std::bind(args...));
	}
};
