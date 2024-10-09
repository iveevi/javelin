#pragma once

#include <thread>

#include <ire/core.hpp>

#include "pipeline_encoding.hpp"
#include "shaders.hpp"
#include "viewport.hpp"

using namespace jvl;
using namespace jvl::ire;

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
	void configure_pipeline_albedo(DeviceResourceCollection &, bool);
	void configure_pipeline_backup(DeviceResourceCollection &, vulkan::VertexFlags);
	void configure_pipelines(DeviceResourceCollection &);

	// Deferred texture loading
	template <typename T>
	bool handle_texture_state(LoadingWork &, T &) {
		return false;
	}

	void texture_loader_worker();

	// Processing descriptor set updates
	void process_descriptor_set_updates(const vk::Device &);

	// Preparations for rendering pipelines
	void prepare_albedo(const RenderingInfo &);

	// Rendering specific groups of pipelines
	void render_albedo(const RenderingInfo &, const Viewport &, const solid_t <MVP> &);
	void render_objects(const RenderingInfo &, const Viewport &, const solid_t <MVP> &);
	void render_default(const RenderingInfo &, const Viewport &, const solid_t <MVP> &);
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
	
	// Rendering
	void render(const RenderingInfo &, Viewport &);

	// Post-frame actions
	void post_render();

	// ImGui rendering
	void popup_debug_pipelines(const RenderingInfo &);

	imgui_callback callback_debug_pipelines();
};