#include <fmt/printf.h>

#include "global_context.hpp"
#include "logging.hpp"

MODULE(global-context);

// Construction	
GlobalContext::GlobalContext(const GlobalContextPrelude &prelude)
{
	drc.phdev = prelude.phdev;
	drc.configure_display(prelude.extent, prelude.title);
	drc.configure_device(prelude.extensions);
}

void GlobalContext::attach(const std::string &key, const Attachment &attachment)
{
	attachments[key] = attachment;
}

// Resizing
void GlobalContext::resize()
{
	drc.combined().resize(drc.surface, drc.window, drc.swapchain);
	for (auto &[k, attachment] : attachments)
		attachment.resizer();
}

void GlobalContext::loop()
{
	// Synchronization information
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	// Main loop
	while (!glfwWindowShouldClose(drc.window.handle)) {
		JVL_INFO("global context render loop: {}", (void *) this);

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

		// Copy attachments in case they change
		auto copied_attachments = attachments;
		for (auto &[k, attachment] : copied_attachments) {
			fmt::println("rendering attachment: {}", k);
			attachment.renderer(rs);
		}
	
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