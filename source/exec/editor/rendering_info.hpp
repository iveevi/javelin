#pragma once

#include <core/device_resource_collection.hpp>
#include <core/scene.hpp>
#include <core/messaging.hpp>
#include <gfx/vulkan/scene.hpp>

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

	// Scenes
	Scene &scene;

	vulkan::Scene &device_scene;

	// Messaging system
	MessageSystem &message_system;
};