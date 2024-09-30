#pragma once

#include <core/scene.hpp>
#include <core/messaging.hpp>
#include <gfx/vulkan/scene.hpp>

#include "gfx.hpp"

using namespace jvl;
using namespace jvl::core;
using namespace jvl::gfx;

struct RenderingInfo {
	// Primary rendering structures
	const vk::CommandBuffer &cmd;
	const DeviceResourceCollection &drc;
	const littlevk::SurfaceOperation &operation;

	InteractiveWindow &window;

	int32_t frame;

	// Scenes
	Scene &scene;

	vulkan::Scene &device_scene;

	// Messaging system
	MessageSystem &message_system;
};