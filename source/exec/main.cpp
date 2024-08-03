#include <cassert>
#include <filesystem>

#include <fmt/printf.h>
#include <fmt/std.h>

#include <littlevk/littlevk.hpp>

#include "core/preset.hpp"

namespace jvl {

struct Kernel {
	enum {
		eCPU,
		eCUDA,
		eVulkan,
		eOpenGL,
		eMetal,
	} backend;
};

} // namespace jvl

#include "core/aperature.hpp"
#include "core/transform.hpp"

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::filesystem::path path = argv[1];
	fmt::println("path to scene: {}", path);

	jvl::core::Preset::from(path);

	//////////////////
	// VULKAN SETUP //
	//////////////////

	// Device extensions
	static const std::vector<const char *> EXTENSIONS{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	vk::PhysicalDevice phdev = littlevk::pick_physical_device(predicate);
}
