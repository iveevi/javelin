#pragma once

#include <imgui/imgui.h>

#include <core/scene.hpp>

#include "rendering_info.hpp"
#include "imgui_render_group.hpp"

using namespace jvl;
using namespace jvl::core;

struct SceneInspector : Unique {
	int64_t selected = -1;

	SceneInspector();

	void scene_hierarchy_object(const Scene &, const Scene::Object &);
	void scene_hierarchy(const RenderingInfo &);
	void object_inspector(const RenderingInfo &);
	imgui_callback callback_scene_hierarchy();
	imgui_callback callback_object_inspector();
};