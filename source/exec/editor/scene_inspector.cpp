#include "scene_inspector.hpp"
#include "core/messaging.hpp"

SceneInspector::SceneInspector() : uuid(new_uuid <SceneInspector> ()) {}

void SceneInspector::scene_hierarchy_object(const Scene &scene, const Scene::Object &obj)
{
	if (obj.children.empty()) {
		if (ImGui::Selectable(obj.name.c_str(), selected == obj.id()))
			selected = obj.id();
	} else {
		ImGuiTreeNodeFlags flags;
		flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (selected == obj.id())
			flags |= ImGuiTreeNodeFlags_Selected;

		bool opened = ImGui::TreeNodeEx(obj.name.c_str(), flags);
		if (ImGui::IsItemClicked())
			selected = obj.id();

		if (opened) {
			for (const auto &uuid : obj.children)
				scene_hierarchy_object(scene, scene[uuid]);

			ImGui::TreePop();
		}
	}
}

void SceneInspector::scene_hierarchy(const RenderingInfo &info)
{
	ImGui::Begin("Scene");

	for (auto &uuid : info.scene.root) {
		auto &obj = info.scene[uuid];
		scene_hierarchy_object(info.scene, obj);
	}

	ImGui::End();

	// Send the update
	Message message {
		.type_id = uuid.type_id,
		.global = uuid.global,
		.kind = editor_update_selected,
		.value = selected,
	};

	info.message_system.send_to_origin(message);
}

void SceneInspector::object_inspector(const RenderingInfo &info)
{
	ImGui::Begin("Inspector");

	if (selected != -1) {
		auto &obj = info.scene[selected];
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

// TODO: should be part of the editor...
imgui_callback SceneInspector::scene_hierarchy_callback()
{
	return {
		-1,
		std::bind(&SceneInspector::scene_hierarchy, this, std::placeholders::_1)
	};
}

imgui_callback SceneInspector::object_inspector_callback()
{
	return {
		-1,
		std::bind(&SceneInspector::object_inspector, this, std::placeholders::_1)
	};
}