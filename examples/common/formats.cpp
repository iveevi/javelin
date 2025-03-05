#include <map>

#include <common/logging.hpp>

#include "formats.hpp"

size_t FormatInfo::bytes(const vk::Extent2D &extent) const
{
	return size * extent.width * extent.height;
}

FormatInfo FormatInfo::fetch(const vk::Format &format)
{
	static const std::map <vk::Format, FormatInfo> format_infos {
		{ vk::Format::eR32Sint, { 4, 1 } },
		{ vk::Format::eR8G8B8Unorm, { 3, 3 } },
		{ vk::Format::eR8G8B8A8Unorm, { 4, 4 } },
		{ vk::Format::eR32G32B32A32Sfloat, { 16, 4 } },
	};

	MODULE(format-info);

	JVL_ASSERT(format_infos.contains(format),
		"unsupported format for readable framebuffer: {}",
		vk::to_string(format));

	return format_infos.at(format);
}