#include "logging.hpp"
#include "gfx/vulkan/texture_bank.hpp"

namespace jvl::gfx::vulkan {

MODULE(texture-bank);

struct FormatInfo {
	size_t size;
	size_t channels;

	size_t bytes(const vk::Extent2D &extent) const {
		return size * extent.width * extent.height;
	}
};

void TextureBank::upload(littlevk::LinkedDeviceAllocator <> allocator,
			 littlevk::LinkedCommandQueue commander,
			 const std::string &path,
			 const core::Texture &texture)
{
	static const std::map <vk::Format, FormatInfo> format_infos {
		{ vk::Format::eR32Sint, { 4, 1 } },
		{ vk::Format::eR8G8B8Unorm, { 3, 3 } },
		{ vk::Format::eR8G8B8A8Unorm, { 4, 4 } },
		{ vk::Format::eR32G32B32A32Sfloat, { 32, 4 } },
	};

	JVL_ASSERT(format_infos.contains(texture.format),
		"unsupported format for readable framebuffer: {}",
		vk::to_string(texture.format));

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

	// TODO: handle sizes...
	std::tie(staging, image) = allocator
		.buffer(texture.data,
			// format_infos.at(texture.format).bytes(vk::Extent2D(texture.width, texture.height)),
			texture.format == vk::Format::eR32G32B32A32Sfloat
			? 4 * texture.width * texture.height * sizeof(float)
			: 4 * texture.width * texture.height * sizeof(char),
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