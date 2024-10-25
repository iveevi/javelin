#pragma once

#include <string>

#include <vulkan/vulkan.hpp>

struct TextureTransitionUnit {
	vk::CommandBuffer cmd;
	std::string source;
};