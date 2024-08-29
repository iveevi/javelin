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
	jvl::wrapped::hash_table< std::string, Attachment> attachments;

	void attach(const std::string &key, const Attachment &attachment) {
		attachments[key] = attachment;
	}

	template <typename T>
	T *attachment_user() {
		return reinterpret_cast <T *> (attachments[T::key].user);
	}

	// Resizing
	void resize() {
		drc.combined().resize(drc.surface, drc.window, drc.swapchain);
		for (auto &[k, attachment] : attachments)
			attachment.resizer();
	}

	// Rendering
	struct {
		int32_t index = 0;
	} frame;

	void loop() {
		// Synchronization information
		auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);
	
		// Command buffers for the rendering loop
		auto command_buffers = littlevk::command_buffers(drc.device,
			drc.command_pool,
			vk::CommandBufferLevel::ePrimary, 2u);

		// Main loop
		while (!glfwWindowShouldClose(drc.window.handle)) {
			// Refresh event list
			glfwPollEvents();
		
			// Grab the next image
			littlevk::SurfaceOperation sop;
			sop = littlevk::acquire_image(drc.device, drc.swapchain.swapchain, sync[frame.index]);
			if (sop.status == littlevk::SurfaceOperation::eResize) {
				resize();
				continue;
			}
		
			// Start the render pass
			const auto &cmd = command_buffers[frame.index];

			vk::CommandBufferBeginInfo cbbi;
			cmd.begin(cbbi);
	
			littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window));

			RenderState rs;
			rs.cmd = cmd;
			rs.frame = frame.index;
			rs.sop = sop;

			for (auto &[k, attachment] : attachments)
				attachment.renderer(rs);
		
			cmd.end();
		
			// Submit command buffer while signaling the semaphore
			constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

			vk::SubmitInfo submit_info {
				sync.image_available[frame.index],
				wait_stage, cmd,
				sync.render_finished[frame.index]
			};

			drc.graphics_queue.submit(submit_info, sync.in_flight[frame.index]);

			// Present to the window
			sop = littlevk::present_image(drc.present_queue, drc.swapchain.swapchain, sync[frame.index], sop.index);
			if (sop.status == littlevk::SurfaceOperation::eResize)
				resize();
				
			// Toggle frame counter
			frame.index = 1 - frame.index;
		}
	}

	// Construction	
	GlobalContext(const GlobalContextPrelude &prelude) {
		drc.phdev = prelude.phdev;
		drc.configure_display(prelude.extent, prelude.title);
		drc.configure_device(prelude.extensions);
	}
};