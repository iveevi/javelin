#include <core/formats.hpp>

#include "logging.hpp"
#include "gfx/vulkan/texture_bank.hpp"

namespace jvl::gfx::vulkan {

static auto allocate(littlevk::LinkedDeviceAllocator <> allocator,
		     const std::string &path,
		     const core::Texture &texture)
{
	littlevk::ImageCreateInfo image_info {
		uint32_t(texture.width),
		uint32_t(texture.height),
		texture.format,
		vk::ImageUsageFlagBits::eSampled
		| vk::ImageUsageFlagBits::eTransferDst,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageType::e2D,
		vk::ImageViewType::e2D
	};

	littlevk::Image image;
	littlevk::Buffer staging;

	auto info = core::FormatInfo::fetch(texture.format);

	std::tie(staging, image) = allocator
		.buffer(texture.data,
			info.bytes(vk::Extent2D(texture.width, texture.height)),
			vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eTransferSrc)
		.image(image_info);

	return std::make_tuple(staging, image);
}

void TextureBank::upload(littlevk::LinkedDeviceAllocator <> allocator,
			 littlevk::LinkedCommandQueue commander,
			 const std::string &path,
			 const core::Texture &texture)
{
	auto [staging, image] = allocate(allocator, path, texture);

	commander.submit_and_wait([&](const vk::CommandBuffer &cmd) {
		image.transition(cmd, vk::ImageLayout::eTransferDstOptimal);

		littlevk::copy_buffer_to_image(cmd,
			image,
			staging,
			vk::ImageLayout::eTransferDstOptimal);

		image.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
	});

	// Assign to the proper spot
	this->operator[](path) = image;
}

void TextureBank::upload(littlevk::LinkedDeviceAllocator <> allocator,
			 littlevk::LinkedCommandQueue commander,
			 const std::string &path,
			 const core::Texture &texture,
			 wrapped::thread_safe_queue <vk::CommandBuffer> &queue)
{
	auto [staging, image] = allocate(allocator, path, texture);

	vk::CommandBuffer cmd = commander.device.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo {
			commander.pool, vk::CommandBufferLevel::ePrimary, 1
		}).front();

	cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
	{
		image.transition(cmd, vk::ImageLayout::eTransferDstOptimal);

		littlevk::copy_buffer_to_image(cmd,
			image,
			staging,
			vk::ImageLayout::eTransferDstOptimal);

		image.transition(cmd, vk::ImageLayout::eShaderReadOnlyOptimal);
	}
	cmd.end();

	queue.push(cmd);

	// Assign to the proper spot
	this->operator[](path) = image;
}

} // namespace jvl::gfx::vulkan