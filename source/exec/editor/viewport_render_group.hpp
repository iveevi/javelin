#pragma once

#include <ire/core.hpp>

#include "viewport.hpp"

using namespace jvl;
using namespace jvl::ire;

// Pipeline encoding
struct PipelineEncoding {
	ViewportMode mode;

	// For albedo, this indicates the material flags
	uint64_t specialization;

	PipelineEncoding(const ViewportMode &mode_, uint64_t specialization_ = 0)
		: mode(mode_), specialization(specialization_) {}

	// Comparison operators
	bool operator==(const PipelineEncoding &other) const {
		return (mode == other.mode)
			&& (specialization == other.specialization);
	}

	bool operator<(const PipelineEncoding &other) const {
		if (mode < other.mode)
			return true;

		return (mode == other.mode) && (specialization < other.specialization);
	}
};

// Deferred texture loading
struct LoadingWork {
	vulkan::Material &material;
	vk::DescriptorSet &descriptor;
	TextureBank &host_bank;
	vulkan::TextureBank &device_bank;
	DeviceResourceCollection &drc;
	wrapped::thread_safe_queue <vk::CommandBuffer> &extra;
	littlevk::Pipeline &ppl;
};

struct DescriptorBinding {
	vk::DescriptorSet descriptor;
	uint32_t binding;
	uint32_t count;
};

struct ImageDescriptorBinding : DescriptorBinding {
	vk::Sampler sampler;
	vk::ImageView view;
	vk::ImageLayout layout;
};

struct BufferDescriptorBinding : DescriptorBinding {
	vk::Buffer buffer;
	uint32_t offset;
	uint32_t range;
};

using GenericDescriptorBinding = wrapped::variant <BufferDescriptorBinding, ImageDescriptorBinding>;

class ViewportRenderGroup {
	void configure_render_pass(DeviceResourceCollection &);
	void configure_pipeline_mode(DeviceResourceCollection &, ViewportMode);
	void configure_pipeline_albedo(DeviceResourceCollection &, vulkan::MaterialFlags);
	void configure_pipelines(DeviceResourceCollection &);
public:
	vk::RenderPass render_pass;

	// Pipeline kinds
	std::map <PipelineEncoding, littlevk::Pipeline> pipelines;

	// Tracking descriptor sets
	std::map <uint64_t, vk::DescriptorSet> descriptors;

	// Thread workers to launch
	enum Worker {
		eTextureLoader,
	};

	std::map <Worker, std::thread> active_workers;
	std::map <Worker, std::atomic <bool>> active_workers_flags;

	// Deferred texture loading
	vk::CommandPool texture_loading_pool;
	wrapped::thread_safe_queue <LoadingWork> work;
	wrapped::thread_safe_queue <LoadingWork> pending;
	wrapped::thread_safe_queue <GenericDescriptorBinding> descriptor_updates;

	// Constructors
	ViewportRenderGroup(DeviceResourceCollection &);

	// Cleaning up
	~ViewportRenderGroup();

	// Deferred texture loading
	template <typename T>
	bool handle_texture_state(LoadingWork &, T &) {
		return false;
	}

	void texture_loader_worker();

	// Processing descriptor set updates
	void process_descriptor_set_updates(const vk::Device &);

	// Rendering
	void render(const RenderingInfo &, Viewport &);

	// Post-frame actions
	void post_render();
};