#include <argparse/argparse.hpp>

#include <littlevk/littlevk.hpp>

#include "vulkan_resources.hpp"
#include "extensions.hpp"
#include "shaders.hpp"

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

// Pipeline compilation
std::optional <littlevk::Pipeline> configure_pipeline()
{
	return std::nullopt;
}

// Argument parsing
argparse::ArgumentParser program("ngf");

int feed(int argc, char *argv[])
{
	program.add_argument("binary")
		.help("neural geometry field binary file");

	program.add_argument("--frames")
		.help("number of frames to run the program for")
		.default_value(-1)
		.scan <'i', int> ();
	
	try {
		program.parse_args(argc, argv);
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << program;
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (int r = feed(argc, argv))
		return r;
	
	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = true;
		config.abort_on_validation_error = true;
	}
	
	std::filesystem::path path = program.get("binary");

	// Configure the resource collection
	auto drc = VulkanResources::from("NGF", vk::Extent2D(1920, 1080), VK_EXTENSIONS);
}