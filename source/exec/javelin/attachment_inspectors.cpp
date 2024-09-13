#include "attachment_inspectors.hpp"

using namespace jvl;
using namespace jvl::core;

// Construction
AttachmentInspectors::AttachmentInspectors(GlobalContext &global_context)
		: scene(global_context.scene) {}

// ImGui rendering
void scene_hierarchy_object(Scene::Object::ref_t &selected, const Scene::Object &obj)
{
	if (obj.children.empty()) {
		if (ImGui::Selectable(obj.name.c_str(), selected == &obj))
			selected = &obj;
	} else {
		ImGuiTreeNodeFlags flags;
		flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (selected == &obj)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool opened = ImGui::TreeNodeEx(obj.name.c_str(), flags);
		if (ImGui::IsItemClicked())
			selected = &obj;

		if (opened) {
			for (const auto &ref : obj.children)
				scene_hierarchy_object(selected, *ref);

			ImGui::TreePop();
		}
	}
}

void AttachmentInspectors::scene_hierarchy()
{
	ImGui::Begin("Scene");

	for (auto &root_ptr: scene.root) {
		auto &obj = *root_ptr;
		scene_hierarchy_object(selected, obj);
	}

	ImGui::End();
};

void AttachmentInspectors::object_inspector()
{
	ImGui::Begin("Inspector");

	if (selected) {
		auto &obj = *selected;
		ImGui::Text("%s", obj.name.c_str());

		if (ImGui::CollapsingHeader("Mesh")) {
			if (obj.geometry) {
				auto &g = obj.geometry.value();
				// TODO: separate method to list all properties
				ImGui::Text("# of vertices: %lu", g.vertex_count);
			}
		}
		
		if (ImGui::CollapsingHeader("Materials")) {
			ImGui::Text("Count: %lu", obj.materials.size());
		}
	}

	ImGui::End();
};