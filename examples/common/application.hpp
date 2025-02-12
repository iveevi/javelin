#pragma once

#include <argparse/argparse.hpp>

#include "vulkan_resources.hpp"

struct VulkanFeatureBase {
	VkStructureType sType;
	void *pNext;
};

struct VulkanFeatureChain : std::vector <VulkanFeatureBase *> {
	vk::PhysicalDeviceFeatures2KHR top;

	~VulkanFeatureChain() {
		for (auto &ptr : *this)
			delete ptr;
	}

	template <typename F>
	void add() {
		VulkanFeatureBase *ptr = (VulkanFeatureBase *) new F();

		// pNext chain
		if (size() > 0) {
			VulkanFeatureBase *pptr = back();
			pptr->pNext = ptr;
		} else {
			top.setPNext(ptr);
		}

		push_back(ptr);
	}
};

#define feature_case(Type)				\
	case (VkStructureType) Type::structureType:	\
		(*reinterpret_cast <Type *> (ptr))

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
};

#define APPLICATION_MAIN()				\
	int main(int argc, char *argv[])		\
	{						\
		return Application().run(argc, argv);	\
	}
