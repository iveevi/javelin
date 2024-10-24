#include <nfd.hpp>

#include <engine/imgui.hpp>
#include <ire/core.hpp>

#include "material_viewer.hpp"

MODULE(material-viewer);

// Material viewer interface
MaterialViewer::MaterialViewer(const Archetype <Material> ::Reference &material_)
		: Unique(new_uuid <MaterialViewer> ()),
		material(material_),
		extent(vk::Extent2D(100, 100))
{
	JVL_ASSERT_PLAIN(extent.old());

	main_title = fmt::format("Material Viewer: {}##{}", material->name, material->id());
}

void MaterialViewer::display_handle(const RenderingInfo &info)
{
	bool property_popup = false;
	bool open = true;

	ImGui::Begin(main_title.c_str(), &open);
	if (!open) {
		auto remove_request = message(editor_remove_self);
		info.message_system.send_to_origin(remove_request);
	}

	ImVec2 available = ImGui::GetContentRegionAvail();
	available.y *= 0.8;
	
	ImGui::Image(descriptor, available);

	// Drop down for other options
	if (ImGui::Button("Add Property"))
		property_popup = true;

	auto next = vk::Extent2D(available.x, available.y);
	next.width = std::max(10u, next.width);
	next.height = std::max(10u, next.height);
	if (extent.current() != next)
		extent.queue(next);

	aperature.aspect = available.x / available.y;

	bool modified = false;
	for (auto &[k, v] : material->values) {
		if (ImGui::CollapsingHeader(k.c_str())) {
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

				if (textures.contains(s)) {
					auto &tex = textures[s];

					vk::DescriptorSet descriptor;
					if (tex.is <TemporaryDescriptor> ())
						descriptor = tex.as <TemporaryDescriptor> ();
					else
						descriptor = tex.as <vk::DescriptorSet> ();

					auto size = ImVec2(available.x, available.x);

					ImGui::Image(descriptor, size);

					static constexpr int length = 25;
					std::string trimmed = s;
					if (trimmed.size() > length)
						trimmed = "..." + trimmed.substr(trimmed.size() - length + 3);

					if (ImGui::Button(trimmed.c_str())) {
						std::filesystem::path path = std::string(s);
						std::string parent = path.parent_path();

						NFD::Guard guard;
						NFD::UniquePath result;
						NFD::OpenDialog(result, nullptr, 0, parent.c_str());
					}
				} else {
					// TODO: loading arrows
					ImGui::Text("Loading...");
				}
			} break;

			variant_case(material_property, name):
			{
				auto &s = v.as <name> ();

				ImGui::Text("%s", s.c_str());
			} break;

			default:
				ImGui::Text("?");
				break;
			}
		}
	}

	if (modified)
		fmt::println("material has changed...");

	ImGui::End();
	
	const char *property_popup_name = "Add Material Property";

	if (ImGui::BeginPopup(property_popup_name)) {
		const std::map <std::string, material_property> properties {
			// Name and default value
			{ Material::ambient_key, color3(0.1, 0.1, 0.1) },
			{ Material::diffuse_key, color3(1, 0, 1) },
			{ Material::emission_key, color3(1, 1, 1) },
			{ Material::roughness_key, 0.4f },
		};

		for (auto &[k, v] : properties) {
			// Grey out if already in properties
			bool disable = material->values.contains(k);
			ImGui::BeginDisabled(disable);
			
			if (ImGui::Selectable(k.c_str())) {
				fmt::println("adding property: \"{}\"", k);
				material->values[k] = v;
			}

			ImGui::EndDisabled();
		}

		ImGui::EndPopup();
	}

	if (property_popup)
		ImGui::OpenPopup(property_popup_name);
}

imgui_callback MaterialViewer::callback_display()
{
	return {
		uuid.global,
		std::bind(&MaterialViewer::display_handle, this, std::placeholders::_1)
	};
}

// Material viewer rendering
MaterialRenderGroup::MaterialRenderGroup(DeviceResourceCollection &drc)
{
	// Only a primary pass
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	// TODO: identical vertex shader program... cache it
}

// Shaders
using namespace jvl::ire;

struct ViewInfo {
	f32 aspect;

	auto layout() const {
		return uniform_layout("ViewInfo",
			named_field(aspect));
	}
};
		
auto bufferless_screen = procedure("main") << []()
{
	// TODO: global arrays... new constructor for forced initialization
	array <vec4> vertex_bank = std::array <vec4, 6> {
		vec4( 1,  1, 0, 1),
		vec4( 1, -1, 0, 1),
		vec4(-1, -1, 0, 1),
		vec4( 1,  1, 0, 1),
		vec4(-1, -1, 0, 1),
		vec4(-1,  1, 0, 1)
	};

	array <vec2> uv_bank = std::array <vec2, 6> {
		vec2(1, 1),
		vec2(1, 0),
		vec2(0, 0),
		vec2(1, 1),
		vec2(0, 0),
		vec2(0, 1)
	};

	layout_out <vec2> uv(0);

	gl_Position = vertex_bank[gl_VertexIndex];
	uv = uv_bank[gl_VertexIndex];
};

void MaterialRenderGroup::render(const RenderingInfo &info, MaterialViewer &viewer)
{
	// Load material texture properties
	for (auto &[k, v] : viewer.material->values) {
		if (!v.is <core::texture> ())
			continue;

		auto &s = v.as <core::texture> ();

		// TODO: need to check for things that need to be loaded
		if (viewer.textures.contains(s)) {
			auto &tex = viewer.textures[s];
			if (tex.is <TemporaryDescriptor> ()) {
				fmt::println("temporary descriptor state...");
				if (info.device_texture_bank.contains(s)) {
					fmt::println("texture is avaiable now!");
			
					auto &image = info.device_texture_bank[s];
					if (!descriptors.contains(*image)) {
						auto sampler = littlevk::SamplerAssembler(info.drc.device, info.drc.dal);
						descriptors[*image] = imgui_texture_descriptor(sampler,
							image.view,
							vk::ImageLayout::eShaderReadOnlyOptimal);
					}

					viewer.textures[s] = descriptors[*image];
				}
			}
		} else {
			static constexpr const char *error_texture = "$checkerboard";
			// Load a dummy picture
			// TODO: dedicated error picture

			auto &image = info.device_texture_bank[error_texture];

			if (!descriptors.contains(*image)) {
				auto sampler = littlevk::SamplerAssembler(info.drc.device, info.drc.dal);
				descriptors[*image] = imgui_texture_descriptor(sampler,
					image.view,
					vk::ImageLayout::eShaderReadOnlyOptimal);
			}
			
			viewer.textures[s] = TemporaryDescriptor(descriptors[*image]);
			// TODO: indicate a loading status
			
			// Meanwhile, load the texture
			auto &texture = core::Texture::from(info.texture_bank, std::string(s));

			info.device_texture_bank.upload(info.drc.allocator(),
				info.drc.commander(),
				std::string(s),
				texture,
				info.extra);
		}
		
	}

	// Delayed construction of the images and framebuffer
	// TODO: resize operation...
	if (viewer.extent.old()) {
		auto &drc = info.drc;

		// if (viewer.image) {
		// 	info.drc.device.destroyImage(*viewer.image);
		// }

		fmt::println("allocating images and creating framebuffer...");

		auto &extent = viewer.extent.updated();

		viewer.image = drc.allocator()
			.image(extent,
				vk::Format::eB8G8R8A8Unorm,
				vk::ImageUsageFlagBits::eColorAttachment
				| vk::ImageUsageFlagBits::eSampled,
				vk::ImageAspectFlagBits::eColor);

		auto framebuffer_info = vk::FramebufferCreateInfo()
			.setRenderPass(render_pass)
			.setAttachmentCount(1)
			.setAttachments(viewer.image.view)
			.setWidth(extent.width)
			.setHeight(extent.height)
			.setLayers(1);

		viewer.framebuffer = drc.device.createFramebuffer(framebuffer_info);
	
		vk::Sampler sampler = littlevk::SamplerAssembler(drc.device, drc.dal);
		
		viewer.descriptor = imgui_texture_descriptor(sampler,
			viewer.image.view,
			vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	// TODO: cache this to avoid duplicates...
	auto converted = gfx::vulkan::Material::from(info.drc, *viewer.material).value();
	auto flags = converted.flags;

	if (!pipelines.contains(flags)) {
		// TODO: move some to push constants
		float vfov = to_radians(viewer.aperature.fovy);

		float3 forward { 0, 0, 1 };
		float3 up { 0, 1, 0 };

		float3 w = normalize(-forward);
		float3 u = normalize(cross(up, w));
		float3 v = cross(w, u);

		auto fs = procedure("main") << [&]() {
			push_constant <ViewInfo> view;

			layout_in <vec2> uv(0);
			
			layout_out <vec4> fragment(0);
        
			float h = std::tan(vfov / 2);

			f32 vheight = 2.0f * h;
			f32 vwidth = vheight * view.aspect;

			vec3 origin = vec3(0, 0, -5);
			vec3 forward = vec3(w.x, w.y, w.z);
			vec3 horizontal = vwidth * vec3(u.x, u.y, u.z);
			vec3 vertical = vheight * vec3(v.x, v.y, v.z);
			vec3 lower_left = origin - horizontal/2.0f - vertical/2.0f - forward;

			vec3 ray = normalize(lower_left + uv.x * horizontal + (1.0f - uv.y) * vertical - origin);
			
			// Intersection with sphere of radius one centered at the origin
			vec3 oc = origin;
			f32 a = dot(ray, ray);
			f32 b = 2 * dot(oc, ray);
			f32 c = dot(oc, oc) - 1;

			f32 discriminant = b * b - 4 * a * c;
			cond(discriminant < 0);
				discard();
			end();

			f32 sd = sqrt(discriminant);

			f32 t = (-b - sd)/(2 * a);
			cond(b + sd > 0);
				t = (-b + sd)/(2 * a);
			end();

			vec3 point = origin + ray * t;
			vec3 normal = normalize(point);

			fragment = vec4(0.5 + 0.5 * normal, 1);
		};

		// TODO: push constants for sphere rotation
		// TODO: uniform buffer for material parameters

		auto vertex_shader = link(bufferless_screen).generate_glsl();
		auto fragment_shader = link(fs).generate_glsl();
		
		fmt::println("{}\n{}", vertex_shader, fragment_shader);

		auto &drc = info.drc;

		auto shaders = littlevk::ShaderStageBundle(drc.device, drc.dal)
			.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);
		
		pipelines[flags] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(shaders)
			.with_push_constant <ViewInfo> (vk::ShaderStageFlagBits::eFragment)
			.cull_mode(vk::CullModeFlagBits::eNone)
			.alpha_blending(false);
	}
	
	auto &cmd = info.cmd;

	// Begin the render pass
	auto &extent = viewer.extent.updated();

	auto viewport = vk::Viewport()
		.setX(0)
		.setY(0)
		.setWidth(extent.width)
		.setHeight(extent.height)
		.setMaxDepth(1.0)
		.setMinDepth(0.0);
	
	auto area = vk::Rect2D()
		.setExtent(extent)
		.setOffset(vk::Offset2D(0, 0));

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, area);

	vk::ClearValue clear = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

	auto begin_info = vk::RenderPassBeginInfo()
		.setRenderPass(render_pass)
		.setRenderArea(area)
		.setClearValues(clear)
		.setFramebuffer(viewer.framebuffer);

	cmd.beginRenderPass(begin_info, vk::SubpassContents::eInline);
	{
		solid_t <ViewInfo> view = viewer.aperature.aspect;

		auto &ppl = pipelines[flags];
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);
		cmd.pushConstants <solid_t <ViewInfo>> (ppl.layout,
			vk::ShaderStageFlagBits::eFragment,
			0, view);
		cmd.draw(6, 1, 0, 0);
	}
	cmd.endRenderPass();
	
	littlevk::transition(info.cmd, viewer.image,
		vk::ImageLayout::ePresentSrcKHR,
		vk::ImageLayout::eShaderReadOnlyOptimal);
}