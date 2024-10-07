#include "gfx/vulkan/texture_bank.hpp"

namespace jvl::gfx::vulkan {

void TextureBank::upload(littlevk::LinkedDeviceAllocator <> allocator,
			 littlevk::LinkedCommandQueue commander,
			 const std::string &path,
			 const core::Texture &texture)
{
	littlevk::ImageCreateInfo image_info {
		uint32_t(texture.width),
		uint32_t(texture.height),
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eSampled
		| vk::ImageUsageFlagBits::eTransferDst,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageType::e2D,
		vk::ImageViewType::e2D
	};

	littlevk::Image image;
	littlevk::Buffer staging;

	// TODO: handle sizes...
	std::tie(staging, image) = allocator
		.buffer(texture.data,
			4 * texture.width * texture.height * sizeof(uint8_t),
			vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eTransferSrc)
		.image(image_info);

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
	littlevk::ImageCreateInfo image_info {
		uint32_t(texture.width),
		uint32_t(texture.height),
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eSampled
		| vk::ImageUsageFlagBits::eTransferDst,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageType::e2D,
		vk::ImageViewType::e2D
	};

	littlevk::Image image;
	littlevk::Buffer staging;

	// TODO: handle sizes...
	std::tie(staging, image) = allocator
		.buffer(texture.data,
			4 * texture.width * texture.height * sizeof(uint8_t),
			vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eTransferSrc)
		.image(image_info);

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