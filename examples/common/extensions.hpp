#pragma once

#include <vulkan/vulkan.hpp>

#define VULKAN_EXTENSIONS(...)					\
	static const std::vector <const char *> VK_EXTENSIONS {	\
		__VA_ARGS__,					\
	}
