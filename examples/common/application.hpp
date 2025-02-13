#pragma once

#include <glm/glm.hpp>

#include <imgui.h>

#include <argparse/argparse.hpp>

#include "aperature.hpp"
#include "camera_controller.hpp"
#include "features.hpp"
#include "transform.hpp"
#include "vulkan_resources.hpp"

struct BaseApplication {
	std::string name;

	VulkanResources resources;

	BaseApplication(const std::string &name_,
			const std::vector <const char *> &extensions,
			void (*features_include)(VulkanFeatureChain &) = nullptr,
			void (*features_activate)(VulkanFeatureChain &) = nullptr) : name(name_)
	{
		// Find a suitable physical device
		auto predicate = [&](vk::PhysicalDevice phdev) {
			return littlevk::physical_device_able(phdev, extensions);
		};

		auto phdev = littlevk::pick_physical_device(predicate);

		// Configure logical device features
		VulkanFeatureChain features;

		if (features_include)
			features_include(features);

		phdev.getFeatures2(&features.top);

		if (features_activate)
			features_activate(features);

		// Initializethe resources
		resources = VulkanResources::from(phdev, name, vk::Extent2D(1920, 1080), extensions, features.top);

		// littlevk configuration
		{
			auto &config = littlevk::config();
			config.enable_logging = true;
			config.abort_on_validation_error = true;
		}

		auto handle = resources.window.handle;
		glfwSetWindowUserPointer(handle, this);
		glfwSetMouseButtonCallback(handle, glfw_button_callback);
		glfwSetCursorPosCallback(handle, glfw_cursor_callback);
	}

	// Render loop resources
	void loop(int32_t max) {
		auto &device = resources.device;
		auto &window = resources.window;
		auto &swapchain = resources.swapchain;
		auto &command_pool = resources.command_pool;
		auto &deallocator = resources.dal;
		
		uint32_t count = swapchain.images.size();
	
		auto sync = littlevk::present_syncronization(device, count).unwrap(deallocator);

		auto command_buffers = device.allocateCommandBuffers({
			command_pool,
			vk::CommandBufferLevel::ePrimary,
			count
		});

		uint32_t frame = 0;
		uint32_t total = 0;

		while (!glfwWindowShouldClose(window.handle)) {
			glfwPollEvents();

			if (total >= max)
				glfwSetWindowShouldClose(window.handle, true);

			littlevk::SurfaceOperation op;
			op = littlevk::acquire_image(device, swapchain.handle, sync[frame]);
			if (op.status == littlevk::SurfaceOperation::eResize) {
				resize();
				continue;
			}
			
			// Record the command buffer
			const auto &cmd = command_buffers[frame];

			cmd.begin(vk::CommandBufferBeginInfo {});
				render(cmd, op.index);
			cmd.end();

			// Submit command buffer while signaling the semaphore
			constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

			vk::SubmitInfo submit_info {
				sync.image_available[frame],
				wait_stage, cmd,
				sync.render_finished[frame]
			};

			resources.graphics_queue.submit(submit_info, sync.in_flight[frame]);

			op = littlevk::present_image(resources.present_queue, swapchain.handle, sync[frame], op.index);
			if (op.status == littlevk::SurfaceOperation::eResize)
				resize();

			frame = (++total) % count;
		}

		device.waitIdle();
	}

	// Launch from arguments
	int run(int argc, char *argv[]) {
		argparse::ArgumentParser program(name);
		
		// Shared arguments
		program.add_argument("--frames")
			.help("number of frames to run the program for")
			.default_value(-1)
			.scan <'i', int32_t> ();

		// Unique arguments
		configure(program);

		// Parse arguments
		try {
			program.parse_args(argc, argv);
		} catch (const std::exception &e) {
			std::cerr << e.what() << std::endl;
			std::cerr << program;
			return 1;
		}

		// Preload data now
		preload(program);

		// Run the program
		loop(program.get <int32_t> ("frames"));

		return 0;
	}

	// Command line arguments configuration
	virtual void configure(argparse::ArgumentParser &) {
		// Nothing extra by default
	}

	// Preload after parsing arguments
	virtual void preload(const argparse::ArgumentParser &) {
		// Do nothing by defaulut
	}

	// Render loop required methods
	virtual void render(const vk::CommandBuffer &, uint32_t) = 0;
	virtual void resize() = 0;

	// Mouse controls
	virtual void mouse_inactive() {}
	virtual void mouse_left_press() {}
	virtual void mouse_left_release() {}
	virtual void mouse_cursor(const glm::vec2 &) {}

	// Default GLFW callbacks
	static void glfw_button_callback(GLFWwindow *window, int button, int action, int mods) {
		BaseApplication *user = reinterpret_cast <BaseApplication *> (glfwGetWindowUserPointer(window));

		ImGuiIO &io = ImGui::GetIO();
		if (io.WantCaptureMouse) {
			io.AddMouseButtonEvent(button, action);
			user->mouse_inactive();
			return;
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS)
				user->mouse_left_press();
			else if (action == GLFW_RELEASE)
				user->mouse_left_release();
		}
	}

	static void glfw_cursor_callback(GLFWwindow *window, double x, double y) {
		ImGuiIO &io = ImGui::GetIO();
		io.MousePos = ImVec2(x, y);

		BaseApplication *user = reinterpret_cast <BaseApplication *> (glfwGetWindowUserPointer(window));

		user->mouse_cursor(glm::vec2(x, y));
	}
};

struct CameraApplication : BaseApplication {
	struct {
		Transform transform;
		Aperature aperature;
		CameraController controller;
	} camera;

	CameraApplication(const std::string &name_,
			const std::vector <const char *> &extensions,
			void (*features_include)(VulkanFeatureChain &) = nullptr,
			void (*features_activate)(VulkanFeatureChain &) = nullptr)
			: BaseApplication(name_, extensions, features_include, features_activate),
			camera { .controller = CameraController(camera.transform, CameraControllerSettings()) } {}

	// Mouse button callbacks
	void mouse_inactive() override {
		camera.controller.voided = true;
		camera.controller.dragging = false;
	}
	
	void mouse_left_press() override {
		camera.controller.dragging = true;
	}
	
	void mouse_left_release() override {
		camera.controller.voided = true;
		camera.controller.dragging = false;
	}

	void mouse_cursor(const glm::vec2 &xy) override {
		camera.controller.handle_cursor(xy);
	}
};

#define APPLICATION_MAIN()				\
	int main(int argc, char *argv[])		\
	{						\
		return Application().run(argc, argv);	\
	}
