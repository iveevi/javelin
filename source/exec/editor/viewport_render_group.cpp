#include <core/color.hpp>
#include <thunder/opt.hpp>

#include "viewport_render_group.hpp"
#include "viewport.hpp"
#include "shaders.hpp"

MODULE(viewport-render-group);

// Constructor	
ViewportRenderGroup::ViewportRenderGroup(DeviceResourceCollection &drc)
{
	configure_render_pass(drc);
	configure_pipelines(drc); // TODO: do at runtime...

	// Configure thread workers		
	auto ftn = std::bind(&ViewportRenderGroup::texture_loader_worker, this);
	auto queue_family = littlevk::find_queue_families(drc.phdev, drc.surface);

	active_workers[eTextureLoader] = std::thread(ftn);
	active_workers_flags[eTextureLoader] = true;

	texture_loading_pool = littlevk::command_pool(drc.device,
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family.graphics
	).unwrap(drc.dal);
}

// Cleaning up
ViewportRenderGroup::~ViewportRenderGroup()
{
	for (auto &[_, f] : active_workers_flags)
		f = false;

	for (auto &[_, t] : active_workers)
		t.join();

	active_workers.clear();
	active_workers_flags.clear();
}

// Configuration
void ViewportRenderGroup::configure_render_pass(DeviceResourceCollection &drc)
{
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();
}

void ViewportRenderGroup::configure_pipeline_mode(DeviceResourceCollection &drc, RenderMode mode)
{
	JVL_ASSERT_PLAIN(mode != RenderMode::eAlbedo);

	JVL_INFO("compiling pipeline for view mode: {}", tbl_render_mode[uint32_t(mode)]);

	auto vertex_layout = littlevk::VertexLayout <
		littlevk::rgb32f,
		littlevk::rgb32f,
		littlevk::rg32f
	> ();

	auto encoding = PipelineEncoding(mode, vulkan::VertexFlags::eAll);

	auto vs_callable = procedure("main") << mode << vertex;
	auto fs_callable = procedure("main") << mode << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	auto ppa = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);

	if (mode == RenderMode::eObject)
		ppa.with_push_constant <solid_t <ObjectInfo>> (vk::ShaderStageFlagBits::eFragment, solid_size <MVP>);
	else
		ppa.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, solid_size <MVP>);

	pipelines[encoding] = ppa;
}

void ViewportRenderGroup::configure_pipeline_albedo(DeviceResourceCollection &drc, bool texture)
{
	fmt::println("compiling pipeline for Albedo (texture={})", texture);

	auto vertex = procedure("main") << []() {
		layout_in <vec3> position(0);
		layout_in <vec2> uv(2);

		layout_out <vec2> out_uv(2);

		// Projection informations
		push_constant <MVP> mvp;
		gl_Position = mvp.project(position);
		gl_Position.y = -gl_Position.y;

		out_uv = uv;
	};

	auto fragment = procedure("main") << [&]() {
		layout_in <vec2> uv(2);

		layout_out <vec4> fragment(0);

		if (texture) {
			sampler2D albedo(0);
			
			push_constant <u32> highlight(solid_size <MVP>);

			fragment = albedo.sample(uv);
			cond(fragment.w < 0.1f);
				discard();
			end();

			fragment.w = 1.0f;

			highlight_fragment(fragment, highlight);
		} else {
			constexpr size_t aligned_mvp_size = ((solid_size <MVP> + 16 - 1) / 16) * 16;

			push_constant <Albedo> albedo(aligned_mvp_size);

			fragment = vec4(albedo.color, 1.0);

			highlight_fragment(fragment, albedo.highlight);
		}
	};
	
	auto vertex_layout = littlevk::VertexLayout <
		littlevk::rgb32f,
		littlevk::rgb32f,
		littlevk::rg32f
	> ();
	
	auto encoding = PipelineEncoding(RenderMode::eAlbedo, vulkan::VertexFlags::eAll, texture);

	std::string vertex_shader = link(vertex).generate_glsl();
	std::string fragment_shader = link(fragment).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	auto ppa = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);
	
	if (texture) {
		ppa.with_dsl_binding(0, vk::DescriptorType::eCombinedImageSampler,
					1, vk::ShaderStageFlagBits::eFragment);

		ppa.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, solid_size <MVP>);
	} else {
		constexpr size_t aligned_mvp_size = ((sizeof(solid_t <MVP>) + 16 - 1) / 16) * 16;

		ppa.with_push_constant <solid_t <Albedo>> (vk::ShaderStageFlagBits::eFragment, aligned_mvp_size);
	}

	pipelines[encoding] = ppa;

	// TODO: auto pipeline construction
}

void ViewportRenderGroup::configure_pipeline_backup(DeviceResourceCollection &drc, vulkan::VertexFlags flags)
{
	auto encoding = PipelineEncoding(RenderMode::eBackup, flags);

	auto [binding, attributes] = vulkan::binding_and_attributes(flags);

	auto vs_callable = procedure("main") << []() {
		layout_in <vec3> position(0);
		// TODO: optimize out unused vertex inputs...
		
		// Projection informations
		push_constant <MVP> mvp;
		gl_Position = mvp.project(position);
		gl_Position.y = -gl_Position.y;
		
		// Regurgitate vertex positions
		layout_out <vec3> out_position(0);

		out_position = position;
	};

	auto fs_callable = procedure("main") << []() {
		layout_out <vec4> fragment(0);

		// Position from vertex shader
		layout_in <vec3> position(0);
		
		// TODO: casting
		f32 modded = mod(floor(position.x) + floor(position.y) + floor(position.z), 2.0f);
		vec3 color(modded);

		fragment = vec4(color, 1);

		// Highlighting the selected object
		push_constant <u32> highlight(sizeof(solid_t <MVP>));

		highlight_fragment(fragment, highlight);
	};

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	auto ppa = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_binding(binding)
		.with_vertex_attributes(attributes)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, solid_size <MVP>)
		.cull_mode(vk::CullModeFlagBits::eNone);

	pipelines[encoding] = ppa;
}

void ViewportRenderGroup::configure_pipelines(DeviceResourceCollection &drc)
{
	for (int32_t i = 0; i < int32_t(RenderMode::eCount); i++) {
		if (i != uint32_t(RenderMode::eAlbedo)
			&& i != uint32_t(RenderMode::eBackup))
			configure_pipeline_mode(drc, (RenderMode) i);
	}
}

// Texture loading thread and states
template <>
bool ViewportRenderGroup::handle_texture_state
	<vulkan::UnloadedTexture> (LoadingWork &unit, vulkan::UnloadedTexture &unloaded)
{
	auto &drc = unit.drc;

	auto &texture = core::Texture::from(unit.host_bank, unloaded.path);

	unit.device_bank.upload(drc.allocator(),
		littlevk::bind(drc.device, texture_loading_pool, drc.graphics_queue),
		unloaded.path,
		texture,
		unit.extra);
	
	unit.material.kd = vulkan::ReadyTexture({}, unloaded.path, false);

	return true;	
}

template <>
bool ViewportRenderGroup::handle_texture_state
		<vulkan::ReadyTexture> (LoadingWork &unit, vulkan::ReadyTexture &ready)
{
	if (ready.updated) {
		unit.descriptor = ready.newer;
		return false;
	} 

	auto &drc = unit.drc;
	
	static std::optional <vk::Sampler> default_sampler;
	if (!default_sampler)
		default_sampler = littlevk::SamplerAssembler(drc.device, drc.dal);

	ready.updated = true;
	ready.newer = littlevk::bind(drc.device, drc.descriptor_pool)
		.allocate_descriptor_sets(*unit.ppl.dsl)
		.front();

	// Push update
	ImageDescriptorBinding update;
	update.descriptor = ready.newer;
	update.binding = 0;
	update.count = 1;
	update.sampler = default_sampler.value();
	update.view = unit.device_bank.at(ready.path).view;
	update.layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	descriptor_updates.push(update);

	return true;
}

void ViewportRenderGroup::texture_loader_worker()
{
	while (active_workers_flags[eTextureLoader]) {
		if (!work.size())
			continue;
		
		auto unit = work.pop();

		auto &kd = unit.material.kd;

		auto ftn = [&](auto &x) {
			if (handle_texture_state(unit, x))
				pending.push(unit);
		};

		std::visit(ftn, kd);
	}
}

// Handling descriptor updates
void ViewportRenderGroup::process_descriptor_set_updates(const vk::Device &device)
{
	std::lock_guard guard(descriptor_updates.lock);

	std::list <vk::DescriptorImageInfo> image_infos;
	std::vector <vk::WriteDescriptorSet> writes;

	while (descriptor_updates.size_locked()) {
		auto update = descriptor_updates.pop_locked();

		// TODO: put the common data into the variant...
		if (auto img_update = update.get <ImageDescriptorBinding> ()) {
			image_infos.emplace_back(img_update->sampler, img_update->view, img_update->layout);

			auto write = vk::WriteDescriptorSet()
				.setDstSet(img_update->descriptor)
				.setDstBinding(img_update->binding)
				.setDescriptorCount(img_update->count)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setImageInfo(image_infos.back());

			writes.emplace_back(write);
		} else if (auto buf_update = update.get <BufferDescriptorBinding> ()) {
			MODULE(viewport-render-group-render);
			JVL_ABORT("unfinished implementation");
		}
	}

	device.updateDescriptorSets(writes, nullptr);
}

// Preparation for rendering
void ViewportRenderGroup::prepare_albedo(const RenderingInfo &info)
{
	auto &drc = info.drc;
	auto &scene = info.device_scene;

	std::set <int> materials;
	for (auto &mesh : scene.meshes) {
		for (auto i : mesh.material_usage) {
			materials.insert(i);
		}
	}

	for (auto i : materials) {
		JVL_ASSERT(i >= 0 && i < int32_t(scene.materials.size()),
			"material index ({}) is out of bounds ({} materials active)",
			i, scene.materials.size());

		auto &material = scene.materials[i];
		if (descriptors.contains(material.id()))
			continue;

		// This only depends on the diffuse texture
		bool texture = enabled(material.flags, vulkan::MaterialFlags::eAlbedoSampler);

		PipelineEncoding encoding(RenderMode::eAlbedo, vulkan::VertexFlags::eAll, texture);

		if (!pipelines.contains(encoding))
			configure_pipeline_albedo(drc, texture);

		if (texture) {
			auto &ppl = pipelines[encoding];

			descriptors[material.uuid.global] = littlevk::bind(drc.device, drc.descriptor_pool)
				.allocate_descriptor_sets(*ppl.dsl)
				.front();

			auto &descriptor = descriptors[material.uuid.global];

			auto sampler = littlevk::SamplerAssembler(drc.device, drc.dal);
			auto binder = littlevk::DescriptorUpdateQueue(descriptor, ppl.bindings);

			if (material.kd.is <vulkan::UnloadedTexture> ()) {
				auto &image = info.device_texture_bank["$checkerboard"];
				binder.queue_update(0, 0, sampler,
					image.view,
					vk::ImageLayout::eShaderReadOnlyOptimal);

				work.push(LoadingWork {
					material,
					descriptor,
					info.texture_bank,
					info.device_texture_bank,
					drc,
					info.extra,
					ppl,
				});
			} else {
				auto &image = material.kd.as <vulkan::LoadedTexture> ();
				binder.queue_update(0, 0, sampler,
					image.view,
					vk::ImageLayout::eShaderReadOnlyOptimal);
			}

			binder.apply(drc.device);
		}
	}
}

// Rendering specific pipelines
void ViewportRenderGroup::render_albedo(const RenderingInfo &info,
					const Viewport &viewport,
					const solid_t <MVP> &mvp)
{
	auto &cmd = info.cmd;
	auto &scene = info.device_scene;

	for (auto &mesh : scene.meshes) {
		auto mid = *mesh.material_usage.begin();

		JVL_ASSERT(mid >= 0 && mid < int32_t(scene.materials.size()),
			"mesh material index ({}) is out of bounds ({} materials active)",
			mid, scene.materials.size());

		auto &material = scene.materials[mid];

		bool texture = enabled(material.flags, vulkan::MaterialFlags::eAlbedoSampler);
		auto encoding = PipelineEncoding(RenderMode::eAlbedo, vulkan::VertexFlags::eAll, texture);
		
		auto &ppl = pipelines[encoding];

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);
		
		cmd.pushConstants <solid_t <MVP>> (ppl.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, mvp);
		
		constexpr size_t aligned_mvp_size = ((sizeof(solid_t <MVP>) + 16 - 1) / 16) * 16;

		if (texture) {
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				ppl.layout, 0,
				descriptors.at(material.uuid.global), {});
		
			cmd.pushConstants <solid_t <u32>> (ppl.layout,
				vk::ShaderStageFlagBits::eFragment,
				solid_size <MVP>,
				viewport.selected == scene.mesh_to_object[mesh]);
		} else {
			solid_t <Albedo> albedo;
			albedo.get <0> () = material.kd.as <float3> ();
			albedo.get <1> () = viewport.selected == scene.mesh_to_object[mesh];

			cmd.pushConstants <solid_t <Albedo>> (ppl.layout,
				vk::ShaderStageFlagBits::eFragment,
				aligned_mvp_size,
				albedo);
		}

		// TODO: custom command buffer extension, with solid_t push constants
		// and drawing meshes like this...
		cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
		cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
	}
}

void ViewportRenderGroup::render_objects(const RenderingInfo &info,
					 const Viewport &viewport,
					 const solid_t <MVP> &mvp)
{
	auto encoding = PipelineEncoding(RenderMode::eObject, vulkan::VertexFlags::eAll);

	auto &cmd = info.cmd;
	auto &scene = info.device_scene;
	auto &ppl = pipelines[encoding];

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

	for (auto &mesh : scene.meshes) {
		solid_t <ObjectInfo> info;
		info.get <0> () = scene.mesh_to_object[mesh];
		info.get <1> () = viewport.selected == info.get <0> ();

		cmd.pushConstants <solid_t <MVP>> (ppl.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, mvp);

		cmd.pushConstants <solid_t <ObjectInfo>> (ppl.layout,
			vk::ShaderStageFlagBits::eFragment,
			sizeof(solid_t <MVP>),
			info);

		cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
		cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
	}
}

void ViewportRenderGroup::render_default(const RenderingInfo &info,
					 const Viewport &viewport,
					 const solid_t <MVP> &mvp)
{
	auto encoding = PipelineEncoding(viewport.mode, vulkan::VertexFlags::eAll);

	auto &cmd = info.cmd;
	auto &scene = info.device_scene;

	auto fetch_pipeline = [&](const vulkan::TriangleMesh &mesh) -> littlevk::Pipeline & {
		if (!vulkan::enabled(mesh.flags, encoding.vertex_flags)) {
			// Try to find the backup pipeline, and create it if not found
			auto encoding = PipelineEncoding(RenderMode::eBackup, mesh.flags);
			if (!pipelines.contains(encoding)) {
				fmt::println("no backup pipeline found, need to recompile...");
				configure_pipeline_backup(info.drc, mesh.flags);
			}
			
			return pipelines.at(encoding);
		}

		return pipelines.at(encoding);
	};

	for (auto &mesh : scene.meshes) {
		// TODO: extend pipeline to one that has uuid, then check if we need to swap?
		// or use the handle itself...
		auto &ppl = fetch_pipeline(mesh);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

		cmd.pushConstants <solid_t <MVP>> (ppl.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, mvp);

		cmd.pushConstants <solid_t <u32>> (ppl.layout,
			vk::ShaderStageFlagBits::eFragment,
			sizeof(solid_t <MVP>),
			viewport.selected == scene.mesh_to_object[mesh]);

		cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
		cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
	}
}

// Rendering
void ViewportRenderGroup::render(const RenderingInfo &info, Viewport &viewport)
{
	auto &cmd = info.cmd;
	auto &drc = info.drc;

	// Process descriptor set updates
	process_descriptor_set_updates(drc.device);

	// Handle mouse input from the window
	viewport.handle_input(info.window);

	// Configure the rendering extent
	littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(viewport.extent));

	// Start the render pass
	littlevk::RenderPassBeginInfo(2)
		.with_render_pass(render_pass)
		.with_framebuffer(viewport.framebuffers[info.operation.index])
		.with_extent(viewport.extent)
		.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
		.clear_depth(1, 1)
		.begin(cmd);

	// Common camera information
	solid_t <MVP> mvp;

	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(viewport.aperature);
	mvp[m_view] = viewport.transform.to_view_matrix();

	// Rendering pipelines
	switch (viewport.mode) {
	case RenderMode::eAlbedo:
		prepare_albedo(info);
		render_albedo(info, viewport, mvp);
		break;
	case RenderMode::eObject:
		render_objects(info, viewport, mvp);
		break;
	default:
		render_default(info, viewport, mvp);
		break;
	}

	cmd.endRenderPass();

	littlevk::transition(info.cmd, viewport.targets[info.operation.index],
		vk::ImageLayout::ePresentSrcKHR,
		vk::ImageLayout::eShaderReadOnlyOptimal);
}

void ViewportRenderGroup::post_render()
{
	// Transfer all pending work to the current work
	while (pending.size()) {
		auto cmd = pending.pop();
		work.push_front(cmd);
	}
}

void ViewportRenderGroup::popup_debug_pipelines(const RenderingInfo &)
{
	if (!ImGui::BeginPopup("Render Pipelines"))
		return;

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 5.0f));

	if (ImGui::BeginTable("pipelines", 3)) {
		// Header
		ImGui::TableSetupColumn("Mode", ImGuiTableColumnFlags_None, 0.0f);
		ImGui::TableSetupColumn("Vertex Flags", ImGuiTableColumnFlags_None, 0.0f);
		ImGui::TableSetupColumn("Specialization", ImGuiTableColumnFlags_None, 0.0f);
		ImGui::TableHeadersRow();

		// Elements
		int row = 0;

		ImU32 color0 = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.0f));
		ImU32 color1 = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f));

		for (auto &[k, _] : pipelines) {
			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, (row++) % 2 ? color0 : color1);

			ImGui::TableNextColumn();
			ImGui::Text("%s", tbl_render_mode[uint32_t(k.mode)]);
			
			ImGui::TableNextColumn();

			auto flags = k.vertex_flags;

			std::vector <std::string> enabled;
			if (vulkan::enabled(flags, vulkan::VertexFlags::ePosition))
				enabled.emplace_back("Vertex");
			if (vulkan::enabled(flags, vulkan::VertexFlags::eNormal))
				enabled.emplace_back("Normal");
			if (vulkan::enabled(flags, vulkan::VertexFlags::eUV))
				enabled.emplace_back("UV");

			std::string enabled_joined;
			for (size_t i = 0; i < enabled.size(); i++) {
				enabled_joined += enabled[i];
				if (i + 1 < enabled.size())
					enabled_joined += ", ";
			}

			ImGui::Text("%s", enabled_joined.c_str());
			
			ImGui::TableNextColumn();
			ImGui::Text("%ju", k.specialization);
		}

		ImGui::EndTable();
	}

	ImGui::PopStyleVar();

	ImGui::EndPopup();
}

imgui_callback ViewportRenderGroup::callback_debug_pipelines()
{
	return {
		-1,
		std::bind(&ViewportRenderGroup::popup_debug_pipelines, this, std::placeholders::_1)
	};
}