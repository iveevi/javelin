#pragma once

#include <littlevk/littlevk.hpp>

#include "vulkan_resources.hpp"

// Assumes ownership of the swapchain (resizing only for now)
struct DefaultFramebufferSet {
	std::optional <littlevk::Image> depth;
	std::vector <vk::Framebuffer> handles;

	const vk::Framebuffer &operator[](size_t i) const {
		return handles[i];
	}

	void resize(VulkanResources &drc, const vk::RenderPass &render_pass) {
		// Handle swapchain resizing here
		drc.combined().resize(drc.surface, drc.window, drc.swapchain);

		// Framebuffer (and depth buffer) resizing here
		auto &extent = drc.window.extent;

		depth = drc.allocator()
			.image(extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		littlevk::FramebufferGenerator generator {
			drc.device, render_pass,
			extent, drc.dal
		};

		for (size_t i = 0; i < drc.swapchain.images.size(); i++)
			generator.add(drc.swapchain.image_views[i], depth->view);

		handles = generator.unpack();
	}
};