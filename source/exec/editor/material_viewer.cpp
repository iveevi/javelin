#include "material_viewer.hpp"

MaterialViewer::MaterialViewer(const Material &material)
		: Unique(new_uuid <MaterialViewer> ()),
		material(material)
{
	main_title = fmt::format("Material Viewer: {}##{}", material.name, material.id());
}

void MaterialViewer::display_handle(const RenderingInfo &info)
{
	bool open = true;
	ImGui::Begin(main_title.c_str(), &open);
	if (!open) {
		auto remove_request = message(editor_remove_self);
		info.message_system.send_to_origin(remove_request);
	}

	ImGui::Text("Properties");
	ImGui::Separator();
	for (auto &[k, v] : material.values) {
		ImGui::Text("%s", k.c_str());
		ImGui::SameLine();

		switch (v.index()) {
		
		variant_case(property_value, float):
		{
			auto &f = v.as <float> ();
			ImGui::Text("%f", f);
		} break;

		variant_case(property_value, float3):
		{
			auto &f = v.as <float3> ();
			ImGui::Text("%f %f %f", f.x, f.y, f.z);
		} break;

		variant_case(property_value, std::string):
		{
			auto &s = v.as <std::string> ();
			ImGui::Text("%s", s.c_str());
		} break;

		default:
			ImGui::Text("?");
			break;
		}
	}

	ImGui::End();
}

imgui_callback MaterialViewer::callback_display()
{
	return {
		uuid.global,
		std::bind(&MaterialViewer::display_handle, this, std::placeholders::_1)
	};
}