#include "formats.hpp"
#include "texture_bank.hpp"

static auto allocate(littlevk::LinkedDeviceAllocator <> allocator,
		     const std::string &path,
		     const Texture &texture)
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

	auto info = FormatInfo::fetch(texture.format);

	std::tie(staging, image) = allocator
		.buffer(texture.data,
			info.bytes(vk::Extent2D(texture.width, texture.height)),
			vk::BufferUsageFlagBits::eTransferDst
			| vk::BufferUsageFlagBits::eTransferSrc)
		.image(image_info);

	return std::make_tuple(staging, image);
}

bool VulkanTextureBank::ready(const std::string &path) const
{
	return !processing.contains(path);
}

void VulkanTextureBank::mark_ready(const std::string &path)
{
	processing.erase(path);
}

bool VulkanTextureBank::upload(littlevk::LinkedDeviceAllocator <> allocator,
			 littlevk::LinkedCommandQueue commander,
			 const std::string &path,
			 const Texture &texture)
{
	if (contains(path))
		return false;

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
	
	return true;
}

bool VulkanTextureBank::upload(littlevk::LinkedDeviceAllocator <> allocator,
			 const vk::CommandBuffer &cmd,
			 const std::string &path,
			 const Texture &texture)
{
	if (contains(path))
		return false;

	auto [staging, image] = allocate(allocator, path, texture);

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

	// Assign to the proper spot
	this->operator[](path) = image;

	// Since we are not waiting for completing, mark as processing
	processing.insert(path);

	return true;
}