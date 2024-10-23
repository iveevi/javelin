#include "material_viewer.hpp"

MaterialViewer::MaterialViewer(Material &material)
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

	bool modified = false;
	for (auto &[k, v] : material.values) {
		ImGui::Text("%s", k.c_str());
		ImGui::SameLine();

		switch (v.index()) {
		
		variant_case(material_property, float):
		{
			auto &f = v.as <float> ();
			ImGui::Text("%f", f);
		} break;

		variant_case(material_property, color3):
		{
			auto &f = v.as <color3> ();
			modified |= ImGui::ColorEdit3("", &f.x);
		} break;

		variant_case(material_property, texture):
		{
			auto &s = v.as <texture> ();
			ImGui::Text("%s", s.c_str());
		} break;

		default:
			ImGui::Text("?");
			break;
		}
	}

	if (modified) {
		fmt::println("material has changed...");
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