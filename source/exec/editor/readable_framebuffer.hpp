#pragma once

#include <logging.hpp>
#include <ire/core.hpp>

#include "viewport.hpp"

using namespace jvl;
using namespace jvl::ire;

struct FormatInfo {
	size_t size;
	size_t channels;

	size_t bytes(const vk::Extent2D &extent) {
		return size * extent.width * extent.height;
	}
};

struct ReadableFramebuffer : Unique {
	Aperature aperature;
	Transform transform;

	littlevk::Image image;
	littlevk::Buffer staging;
	littlevk::Pipeline pipeline;
	vk::Extent2D extent;

	vk::RenderPass render_pass;
	vk::Framebuffer framebuffer;

	// Requests
	struct ObjectSelection {
		int2 pixel;

		size_t index(const vk::Extent2D &extent) const {
			return pixel.x + extent.width * pixel.y;
		}
	};

	ObjectSelection request;

	// Doing the request
	struct Null {};
	struct WaitCopy {};
	struct WaitRead {};

	wrapped::variant <Null, WaitCopy, WaitRead> stage;

	ReadableFramebuffer(DeviceResourceCollection &,
			    const Viewport &,
			    const vk::Format &,
			    const vk::Extent2D &);

	void apply_request(const ObjectSelection &r);

	void go(const RenderingInfo &, Null);
	void go(const RenderingInfo &, WaitCopy);
	void go(const RenderingInfo &, WaitRead);
	void go(const RenderingInfo &);
};