#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <ire/core.hpp>

#include "extensions.hpp"
#include "default_framebuffer_set.hpp"
#include "aperature.hpp"
#include "device_resource_collection.hpp"
#include "scene.hpp"
#include "transform.hpp"
#include "camera_controller.hpp"
#include "imgui.hpp"
#include "imported_asset.hpp"
#include "cpu/scene.hpp"
#include "vulkan/scene.hpp"

using namespace jvl;
using namespace jvl::ire;

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto controller = reinterpret_cast <CameraController *> (glfwGetWindowUserPointer(window));

	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		io.AddMouseButtonEvent(button, action);
		controller->voided = true;
		controller->dragging = false;
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			controller->dragging = true;
		} else if (action == GLFW_RELEASE) {
			controller->dragging = false;
			controller->voided = true;
		}
	}
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	ImGuiIO &io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);

	auto controller = reinterpret_cast <CameraController *> (glfwGetWindowUserPointer(window));
	controller->handle_cursor(glm::vec2(x, y));
}

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

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	// Configure the resource collection
	DeviceResourceCollectionInfo info {
		.phdev = littlevk::pick_physical_device(predicate),
		.title = "Raytracing",
		.extent = vk::Extent2D(1920, 1080),
		.extensions = VK_EXTENSIONS,
	};
	
	auto drc = DeviceResourceCollection::from(info);

	// Load the scene
	auto asset = engine::ImportedAsset::from(path).value();
	auto scene = core::Scene();
	scene.add(asset);

	// Prepare host and device scenes
	auto host_scene = gfx::cpu::Scene::from(scene);
	auto vk_scene = gfx::vulkan::Scene::from(drc, host_scene, gfx::vulkan::SceneFlags::eDefault);

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	// Configure ImGui
	engine::configure_imgui(drc, render_pass);

	auto &sampled = vk_scene.meshes[0];
	// auto ppl = configure_pipeline(drc,
	// 	sampled.flags,
	// 	render_pass,
	// 	saturation,
	// 	lightness,
	// 	splits);
	
	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperature
	Transform camera_transform;
	Transform model_transform;
	Aperature aperature;
}