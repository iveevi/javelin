#pragma once

#include "device_resource_collection.hpp"

namespace jvl::engine {

struct FrameRenderContext {
	const core::DeviceResourceCollection &drc;
	const vk::CommandBuffer &cmd;
	const littlevk::PresentSyncronization::Frame &sync_frame;
	std::function <void ()> resizer;

	littlevk::SurfaceOperation sop;

	FrameRenderContext(const core::DeviceResourceCollection &drc_,
			   const vk::CommandBuffer &cmd_,
			   const littlevk::PresentSyncronization::Frame &sync_frame_,
			   const std::function <void ()> &resizer_)
		: drc(drc_), cmd(cmd_), sync_frame(sync_frame_), resizer(resizer_) {
		sop = littlevk::acquire_image(drc.device, drc.swapchain.handle, sync_frame);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			resizer();
	}

	~FrameRenderContext() {
		// Present to the window
		sop = littlevk::present_image(drc.present_queue, drc.swapchain.handle, sync_frame, sop.index);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			resizer();
	}

	operator bool() const {
		return (sop.status == littlevk::SurfaceOperation::eOk);
	}
};

} // namespace jvl::engine