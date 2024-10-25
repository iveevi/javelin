#pragma once

#include <core/device_resource_collection.hpp>
#include <core/scene.hpp>
#include <core/texture.hpp>
#include <core/messaging.hpp>
#include <gfx/vulkan/scene.hpp>
#include <gfx/vulkan/texture_bank.hpp>

#include "texture_transition.hpp"
#include "texture_loading.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::gfx;
using namespace jvl::engine;

struct RenderingInfo {
	// Primary rendering structures
	DeviceResourceCollection &drc;
	const vk::CommandBuffer &cmd;
	const littlevk::SurfaceOperation &operation;
	InteractiveWindow &window;
	int32_t frame;

	// Caches
	TextureBank &texture_bank;
	vulkan::TextureBank &device_texture_bank;

	// Scenes
	Scene &scene;
	vulkan::Scene &device_scene;

	// Messaging system
	MessageSystem &message_system;

	// Worker threads
	std::unique_ptr <TextureLoadingWorker> &worker_texture_loading;

	// Additional command buffers for the frame
	wrapped::thread_safe_queue <TextureTransitionUnit> &transitions;
};