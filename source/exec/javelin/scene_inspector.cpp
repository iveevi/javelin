#include "scene_inspector.hpp"
#include "source/exec/javelin/imgui_render_group.hpp"

void SceneInspector::scene_hierarchy_object(const Scene::Object &obj)
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
				scene_hierarchy_object(*ref);

			ImGui::TreePop();
		}
	}
}

void SceneInspector::scene_hierarchy(const RenderingInfo &info)
{
	ImGui::Begin("Scene");

	for (auto &root_ptr: info.scene.root) {
		auto &obj = *root_ptr;
		scene_hierarchy_object(obj);
	}

	ImGui::End();
}

void SceneInspector::object_inspector(const RenderingInfo &info)
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

				auto &triangles = g.face_properties.at(Mesh::triangle_key);
				ImGui::Text("# of triangles: %lu", typed_buffer_size(triangles));
			}
		}
		
		if (ImGui::CollapsingHeader("Materials")) {
			ImGui::Text("Count: %lu", obj.materials.size());
		}
	}

	ImGui::End();
}

imgui_callback SceneInspector::scene_hierarchy_callback()
{
	return std::bind(&SceneInspector::scene_hierarchy, this, std::placeholders::_1);
}

imgui_callback SceneInspector::object_inspector_callback()
{
	return std::bind(&SceneInspector::object_inspector, this, std::placeholders::_1);
}