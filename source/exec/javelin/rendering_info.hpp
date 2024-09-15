#pragma once

#include <core/scene.hpp>
#include <gfx/vulkan/scene.hpp>

#include "device_resource_collection.hpp"
#include "source/exec/javelin/messaging.hpp"

using namespace jvl;
using namespace jvl::gfx;

struct RenderingInfo {
	// Primary rendering structures
	const vk::CommandBuffer &cmd;
	const littlevk::SurfaceOperation &operation;

	InteractiveWindow &window;

	int32_t frame;

	// Scenes
	core::Scene &scene;

	vulkan::Scene &device_scene;

	// Messaging system
	MessageSystem &message_system;
};