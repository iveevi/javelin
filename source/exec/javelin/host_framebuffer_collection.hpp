#pragma once

#include "device_resource_collection.hpp"

#include "gfx/cpu/framebuffer.hpp"

struct HostFramebufferCollection {
	jvl::gfx::cpu::Framebuffer <jvl::float4> host;
	littlevk::Buffer staging;
	littlevk::Image display;
	vk::DescriptorSet descriptor;
	vk::Sampler sampler;

	littlevk::LinkedDeviceAllocator <> allocator;
	littlevk::LinkedCommandQueue commander;

	HostFramebufferCollection(littlevk::LinkedDeviceAllocator <> allocator_,
			     	  littlevk::LinkedCommandQueue commander_)
			: allocator(allocator_),
				commander(commander_) {
		sampler = littlevk::SamplerAssembler(allocator.device, allocator.dal);
	}

	void __copy(const vk::CommandBuffer &cmd) {
		display.transition(cmd, vk::ImageLayout::eTransferDstOptimal);

		littlevk::copy_buffer_to_image(cmd,
			display, staging,
			vk::ImageLayout::eTransferDstOptimal);

		display.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	void __allocate() {
		std::tie(staging, display) = allocator
			.buffer(host.data, vk::BufferUsageFlagBits::eTransferSrc)
			.image(vk::Extent2D {
					uint32_t(host.width),
					uint32_t(host.height)
				},
				// vk::Format::eR8G8B8A8Unorm,
				vk::Format::eR32G32B32A32Sfloat,
				vk::ImageUsageFlagBits::eSampled
					| vk::ImageUsageFlagBits::eTransferDst,
				vk::ImageAspectFlagBits::eColor);

		commander.submit_and_wait([&](const vk::CommandBuffer &cmd) {
			__copy(cmd);
		});

		descriptor = imgui_add_vk_texture(sampler, display.view, vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	void resize(size_t width, size_t height) {
		host.resize(width, height);
		__allocate();
	}

	void refresh(const vk::CommandBuffer &cmd) {
		if (empty())
			return;

		littlevk::upload(allocator.device, staging, host.data);
		__copy(cmd);
	}

	bool empty() {
		return (host.width == 0) || (host.height == 0);
	}
};
