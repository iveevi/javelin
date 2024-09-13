#pragma once

#include "global_context.hpp"

struct AttachmentInspectors {
	// Active scene
	jvl::core::Scene &scene;

	// Currently highlighted object
	jvl::core::Scene::Object::ref_t selected = nullptr;

	// Construction
	AttachmentInspectors(GlobalContext &);

	// ImGui rendering
	void scene_hierarchy();
	void object_inspector();
};