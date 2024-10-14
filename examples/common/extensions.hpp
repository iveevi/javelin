#pragma once

#define VULKAN_EXTENSIONS(...)					\
	static const std::vector <const char *> VK_EXTENSIONS {	\
		__VA_ARGS__,					\
	}
