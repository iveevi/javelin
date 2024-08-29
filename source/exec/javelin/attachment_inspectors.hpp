#pragma once

#include "global_context.hpp"

struct AttachmentInspectors {
	// Active scene
	jvl::core::Scene &scene;

	// Currently highlighted object
	int32_t selected = -1;

	// Construction
	AttachmentInspectors(const std::unique_ptr <GlobalContext> &);

	// ImGui rendering
	void scene_hierarchy();
	void object_inspector();
};