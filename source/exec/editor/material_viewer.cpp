#include <nfd.hpp>

#include <imgui/imgui_internal.h>

#include <engine/imgui.hpp>
#include <ire/core.hpp>

#include "material_viewer.hpp"
	
static constexpr const char *main_ppl_key = "$main";

MODULE(material-viewer);

// Material viewer interface
MaterialViewer::MaterialViewer(const Archetype <Material> ::Reference &material_)
		: Unique(new_uuid <MaterialViewer> ()),
		material(material_),
		extent(vk::Extent2D(100, 100))
{
	JVL_ASSERT_PLAIN(extent.old());

	main_title = fmt::format("Material Viewer: {}##{}", material->name, material->id());

	// Populate the solo textures list with properties which are textures
	for (auto &[k, v] : material->values) {
		if (v.is <core::texture> ())
			descriptors[k] = AdaptiveDescriptor();
	}

	descriptors[main_ppl_key] = AdaptiveDescriptor();
}

void MaterialViewer::display_handle(const RenderingInfo &info)
{
	bool popup_properties = false;
	bool open = true;

	ImGui::Begin(main_title.c_str(), &open);
	if (!open) {
		auto remove_request = message(editor_remove_self);
		info.message_system.send_to_origin(remove_request);
	}

	ImVec2 available = ImGui::GetContentRegionAvail();
	available.y *= 0.8;
	
	ImGui::ImageButton(viewport, available, ImVec2(0, 0), ImVec2(1, 1), 0);
	ImGui::SetItemUsingMouseWheel();
	
	// Input handling
	static constexpr float pad = 0.1f;

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

		theta += 0.01f * delta.x;
		phi += 0.01f * delta.y;

		phi = std::clamp(phi, pad, pi <float> - pad);

		if (!dragging) {
			glfwSetInputMode(info.drc.window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			dragging = true;
		}

		ImGui::ResetMouseDragDelta();
	} else {
		glfwSetInputMode(info.drc.window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		dragging = false;
	}

	if (ImGui::IsItemHovered()) {
		float scrolling = ImGui::GetIO().MouseWheel;
		
		radius += 0.1f * scrolling;
		radius = std::clamp(radius, 1.0f + pad, 10.0f);
	}

	auto next = vk::Extent2D(available.x, available.y);
	next.width = std::max(10u, next.width);
	next.height = std::max(10u, next.height);
	if (extent.current() != next)
		extent.queue(next);

	aspect = available.x / available.y;

	// Drop down for other options
	if (ImGui::Button("Add Property"))
		popup_properties = true;

	bool modified = false;
	for (auto &[k, v] : material->values) {
		if (ImGui::CollapsingHeader(k.c_str())) {
			switch (v.index()) {
			
			variant_case(material_property, float):
			{
				auto &f = v.as <float> ();

				if (k == Material::roughness_key)
					ImGui::SliderFloat("Value", &f, 0, 1);
				else
					ImGui::Text("%f", f);
			} break;

			variant_case(material_property, color3):
			{
				auto &f = v.as <color3> ();
				modified |= ImGui::ColorEdit3("", &f.x);
			} break;

			variant_case(material_property, texture):
			{
				static constexpr int length = 25;

				std::string source = v.as <core::texture> ();
				std::string trimmed = source;
				if (trimmed.size() > length)
					trimmed = "..." + trimmed.substr(trimmed.size() - length + 3);

				auto size = ImVec2(available.x, available.x);
				ImGui::Image(descriptors[k].set(), size);

				if (ImGui::Button(trimmed.c_str())) {
					std::filesystem::path path = std::string(source);
					std::string parent = path.parent_path();

					NFD::Guard guard;
					NFD::UniquePath result;
					NFD::OpenDialog(result, nullptr, 0, parent.c_str());
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
	
	const char *popup_properties_name = "Add Material Property";

	if (ImGui::BeginPopup(popup_properties_name)) {
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

	if (popup_properties)
		ImGui::OpenPopup(popup_properties_name);
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

struct Ray {
	vec3 d;
	vec3 origin;

	vec3 at(f32 t) const {
		return origin + t * d;
	}

	auto layout() const {
		return uniform_layout("Ray",
			named_field(d),
			named_field(origin));
	}
};

struct ViewInfo {
	f32 aspect;
	f32 theta;
	f32 phi;
	f32 radius;

	auto ray(vec2 uv) const {
		static constexpr float fov = 45.0f;

		float vfov = to_radians(fov);
		float h = std::tan(vfov / 2);

		f32 vheight = 2.0f * h;
		f32 vwidth = vheight * aspect;

		vec3 fwd;
		fwd.x = sin(phi) * cos(theta);
		fwd.y = cos(phi);
		fwd.z = sin(phi) * sin(theta);

		vec3 origin = -radius * fwd;

		vec3 w = normalize(-fwd);
		vec3 u = normalize(cross(vec3(0, 1, 0), w));
		vec3 v = cross(w, u);

		vec3 forward = vec3(w.x, w.y, w.z);
		vec3 horizontal = vwidth * vec3(u.x, u.y, u.z);
		vec3 vertical = vheight * vec3(v.x, v.y, v.z);
		vec3 lower_left = origin - horizontal/2.0f - vertical/2.0f - forward;

		vec3 direction = normalize(lower_left + uv.x * horizontal + (1.0f - uv.y) * vertical - origin);

		return ::Ray(direction, origin);
	}

	auto layout() const {
		return uniform_layout("ViewInfo",
			named_field(aspect),
			named_field(theta),
			named_field(phi),
			named_field(radius));
	}
};

// TODO: as procedure
auto direction_uv = procedure("direction_uv") << [](vec3 d)
{
	f32 u = atan(d.x, d.z) / (2.0f * pi <float>) + 0.5f;
	f32 v = 0.5f + 0.5f * d.y;
	return vec2(u, 1 - v);
};

auto ray_sphere = procedure <optional <f32>> ("ray_sphere") << [](::Ray ray)
{
	// Intersection with sphere of radius one centered at the origin
	vec3 oc = ray.origin;
	f32 a = dot(ray.d, ray.d);
	f32 b = 2 * dot(oc, ray.d);
	f32 c = dot(oc, oc) - 1;

	f32 discriminant = b * b - 4 * a * c;
	cond(discriminant < 0);
		returns(optional <f32> (std::nullopt));
	end();

	f32 sd = sqrt(discriminant);

	f32 t = (-b - sd)/(2 * a);
	cond(b + sd > 0);
		t = (-b + sd)/(2 * a);
	end();

	returns(optional <f32> (t));
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
	// Load the environment if not already loaded
	static constexpr const char *environment = "resources/textures/green_sanctuary_4k.exr";

	// Delayed construction of the images and framebuffer
	// TODO: resize operation...
	if (viewer.extent.old()) {
		auto &drc = info.drc;

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
		
		viewer.viewport = imgui_texture_descriptor(sampler,
			viewer.image.view,
			vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	// TODO: cache in each viewer this to avoid duplicates...
	// TODO: derive each material viewer from the vulkan material type..
	auto converted = gfx::vulkan::Material(*viewer.material);
	auto flags = converted.flags;

	static constexpr size_t B_ENVIRONMENT = 0;
	static constexpr size_t B_ALBEDO = 1;

	if (!pipelines.contains(flags)) {
		bool albedo_sampler = vulkan::enabled(flags, vulkan::MaterialFlags::eAlbedoSampler);

		auto fs = procedure("main") << [albedo_sampler]() {
			push_constant <ViewInfo> view;

			sampler2D environment(B_ENVIRONMENT);
			sampler2D albedo(B_ALBEDO);

			layout_in <vec2> uv(0);
			
			layout_out <vec4> fragment(0);

			auto ray = view.ray(uv);

			optional <f32> t = ray_sphere(ray);
			cond(!t.has_value());
			{
				vec2 uv = direction_uv(ray.d);
				fragment = environment.sample(uv);
				fragment.w = 1.0f;
			}
			elif();
			{
				vec3 point = ray.at(t.value());
				vec3 normal = normalize(point);
				
				vec2 uv = direction_uv(normal);

				if (albedo_sampler) {
					fragment = albedo.sample(uv);
					fragment.w = 1.0f;
				} else {
					fragment = vec4(0.5 + 0.5 * normal, 1);
				}
			}
			end();
		};

		// TODO: push constants for sphere rotation
		// TODO: uniform buffer for material parameters

		fs.dump();
		fmt::println("(^) full shader (albedo={})", albedo_sampler);

		auto vertex_shader = link(bufferless_screen).generate_glsl();
		auto fragment_shader = link(fs).generate_glsl();
		
		fmt::println("{}\n{}", vertex_shader, fragment_shader);

		auto &drc = info.drc;

		auto shaders = littlevk::ShaderStageBundle(drc.device, drc.dal)
			.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);
		
		auto assembler = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(shaders)
			.with_push_constant <ViewInfo> (vk::ShaderStageFlagBits::eFragment)
			.with_dsl_binding(0, vk::DescriptorType::eCombinedImageSampler,
					  1, vk::ShaderStageFlagBits::eFragment)
			.cull_mode(vk::CullModeFlagBits::eNone)
			.alpha_blending(false);

		if (albedo_sampler) {
			assembler.with_dsl_binding(1, vk::DescriptorType::eCombinedImageSampler,
						   1, vk::ShaderStageFlagBits::eFragment);
		}
		
		pipelines[flags] = assembler;
	}

	auto &ppl = pipelines[flags];
	
	// Check all descriptors
	for (auto &[entry, descriptor] : viewer.descriptors) {
		if (descriptor.null()) {
			auto backup_texture = info.device_texture_bank["$checkerboard"];
			auto sampler = littlevk::SamplerAssembler(info.drc.device, info.drc.dal);

			// TODO: this part depends on the entry...
			if (entry == main_ppl_key) {
				fmt::println("null descriptor, starting initialization process");

				descriptor // ...
					.with_layout(ppl.dsl.value())
					.allocate(info.drc.device, info.drc.descriptor_pool);

				auto binder = littlevk::DescriptorUpdateQueue(descriptor.set(), ppl.bindings)
					.queue_update(B_ENVIRONMENT, 0, sampler,
						backup_texture.view,
						vk::ImageLayout::eShaderReadOnlyOptimal);

				if (vulkan::enabled(flags, vulkan::MaterialFlags::eAlbedoSampler)) {
					binder.queue_update(B_ALBEDO, 0, sampler,
						backup_texture.view,
						vk::ImageLayout::eShaderReadOnlyOptimal);
				}

				binder.apply(info.drc.device);

				descriptor.requirement(AdaptiveDescriptor::environment_key, B_ENVIRONMENT);
				descriptor.requirement(Material::diffuse_key, B_ALBEDO);
			} else {
				auto binding = vk::DescriptorSetLayoutBinding()
					.setBinding(B_ENVIRONMENT)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eFragment)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

				auto layout_info = vk::DescriptorSetLayoutCreateInfo()
					.setBindings(binding);

				auto layout = info.drc.device.createDescriptorSetLayout(layout_info);

				descriptor.with_layout(layout)
					.allocate(info.drc.device, info.drc.descriptor_pool);

				descriptor.requirement(entry, B_ENVIRONMENT);
			}
		} else if (!descriptor.complete()) {
			// TODO: this function is common...

			// Manage descriptor set updates upon texture loading
			for (auto &[key, updating] : descriptor.dependencies()) {
				std::string source;
				if (key == AdaptiveDescriptor::environment_key) {
					// TODO: detach from the material descriptor set...
					source = environment;
				} else {
					JVL_ASSERT(viewer.material->values.contains(key),
						"material does not have key \'{}\'", key);

					source = viewer.material->values[key].as <core::texture> ();
				}

				if (info.device_texture_bank.contains(source)
					&& info.device_texture_bank.ready(source)) {
					auto sampler = littlevk::SamplerAssembler(info.drc.device, info.drc.dal);
					auto image = info.device_texture_bank[source];
					
					AdaptiveDescriptor old_descriptor = descriptor;

					descriptor.allocate(info.drc.device, info.drc.descriptor_pool);

					// Copy old descriptor set
					std::vector <vk::CopyDescriptorSet> copies;
					vk::WriteDescriptorSet write;

					for (auto &[_, binding] : descriptor.mapped()) {
						if (binding != updating) {
							auto copy_info = vk::CopyDescriptorSet()
								.setDescriptorCount(1)
								.setDstArrayElement(0)
								.setDstBinding(binding)
								.setDstSet(descriptor.set())
								.setSrcArrayElement(0)
								.setSrcBinding(binding)
								.setSrcSet(old_descriptor.set());

							copies.push_back(copy_info);
						} else {
							auto image_info = vk::DescriptorImageInfo()
								.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
								.setImageView(image.view)
								.setSampler(sampler);

							write = vk::WriteDescriptorSet()
								.setDescriptorCount(1)
								.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
								.setImageInfo(image_info)
								.setDstSet(descriptor.set())
								.setDstArrayElement(0)
								.setDstBinding(descriptor.key_binding(key));
						}
					}

					info.drc.device.updateDescriptorSets(write, copies);

					descriptor.resolve(key);
				} else if (!info.worker_texture_loading->pending(source)) {
					TextureLoadingUnit unit { source, true };
					info.worker_texture_loading->push(unit);
				}
			}
		}
	}
	
	auto &descriptor = viewer.descriptors[main_ppl_key];
	
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
		solid_t <ViewInfo> view;
		view.get <0> () = viewer.aspect;
		view.get <1> () = viewer.theta;
		view.get <2> () = viewer.phi;
		view.get <3> () = viewer.radius;

		auto &ppl = pipelines[flags];

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			ppl.layout, 0u,
			descriptor.set(), {});

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