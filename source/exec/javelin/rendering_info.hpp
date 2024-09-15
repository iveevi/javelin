#pragma once

#include <core/scene.hpp>
#include <core/messaging.hpp>
#include <gfx/vulkan/scene.hpp>

#include "device_resource_collection.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::gfx;

struct RenderingInfo {
	// Primary rendering structures
	const vk::CommandBuffer &cmd;
	const littlevk::SurfaceOperation &operation;

	InteractiveWindow &window;

	int32_t frame;

	// Scenes
	Scene &scene;

	vulkan::Scene &device_scene;

	// Messaging system
	MessageSystem &message_system;
};