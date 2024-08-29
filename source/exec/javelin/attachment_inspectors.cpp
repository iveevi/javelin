#include "attachment_inspectors.hpp"

// Construction
AttachmentInspectors::AttachmentInspectors(const std::unique_ptr <GlobalContext> &global_context)
		: scene(global_context->scene) {}

// ImGui rendering
void AttachmentInspectors::scene_hierarchy()
{
	ImGui::Begin("Scene");

	for (int32_t i = 0; i < scene.objects.size(); i++) {
		if (ImGui::Selectable(scene.objects[i].name.c_str(), selected == i)) {
			selected = i;
		}
	}

	ImGui::End();
};

void AttachmentInspectors::object_inspector()
{
	ImGui::Begin("Inspector");

	if (selected >= 0) {
		auto &obj = scene.objects[selected];

		ImGui::Text("%s", obj.name.c_str());

		if (ImGui::CollapsingHeader("Meshes")) {}
		
		if (ImGui::CollapsingHeader("Materials")) {
			ImGui::Text("Count: %lu", obj.materials.size());
		}
	}

	ImGui::End();
};