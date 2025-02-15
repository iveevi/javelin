#include <ire.hpp>

#include "common/extensions.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/aperature.hpp"
#include "common/vulkan_resources.hpp"
#include "common/transform.hpp"
#include "common/camera_controller.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"

using namespace jvl;
using namespace jvl::ire;

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}
	
	// Load the asset and scene
	std::filesystem::path path = argv[1];

	// Configure the resource collection
	auto predicate = [&](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	auto phdev = littlevk::pick_physical_device(predicate);

	auto drc = VulkanResources::from(phdev, "Raytracing", vk::Extent2D(1920, 1080), VK_EXTENSIONS);

	// Load the scene
	auto asset = ImportedAsset::from(path).value();

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	// Configure ImGui
	configure_imgui(drc, render_pass);

	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperature
	Transform camera_transform;
	Transform model_transform;
	Aperature aperature;
}