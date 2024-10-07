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
	configure_pipelines(drc);

	// Configure thread workers		
	auto ftn = std::bind(&ViewportRenderGroup::texture_loader_worker, this);
	auto queue_family = littlevk::find_queue_families(drc.phdev, drc.surface);

	active_workers[eTextureLoader] = std::thread(ftn);

	texture_loading_pool = littlevk::command_pool(drc.device,
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family.graphics
	).unwrap(drc.dal);
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

void ViewportRenderGroup::configure_pipeline_mode(DeviceResourceCollection &drc, ViewportMode mode)
{
	JVL_ASSERT_PLAIN(mode != ViewportMode::eAlbedo);

	auto vertex_layout = littlevk::VertexLayout <
		littlevk::rgb32f,
		littlevk::rgb32f,
		littlevk::rg32f
	> ();

	auto vs_callable = callable("main") << mode << vertex;
	auto fs_callable = callable("main") << mode << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	pipelines[mode] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void ViewportRenderGroup::configure_pipelines(DeviceResourceCollection &drc)
{
	for (int32_t i = 0; i < uint32_t(ViewportMode::eCount); i++) {
		if (i != uint32_t(ViewportMode::eAlbedo))
			configure_pipeline_mode(drc, (ViewportMode) i);
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
		// TODO: destroy old descriptor set
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
	while (true) {
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

// Rendering
void ViewportRenderGroup::render(const RenderingInfo &info, Viewport &viewport)
{
	auto &cmd = info.cmd;
	auto &drc = info.drc;
	auto &scene = info.device_scene;

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
	auto m_id = uniform_field(MVP, id);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(viewport.aperature);
	mvp[m_view] = viewport.transform.to_view_matrix();

	// Pre-rendering actions
	if (viewport.mode == ViewportMode::eAlbedo) {
		for (size_t i = 0; i < scene.uuids.size(); i++) {
			auto &mesh = scene.meshes[i];
			for (auto i : mesh.material_usage) {
				// TODO: may need multiple descriptors per material
				auto &material = scene.materials[i];
				if (descriptors.contains(material.uuid.global))
					continue;

				// This only depends on the diffuse texture
				bool texture = enabled(material.flags, vulkan::MaterialFlags::eAlbedoSampler);

				PipelineEncoding encoding(ViewportMode::eAlbedo, texture);

				if (!pipelines.contains(encoding)) {
					fmt::println("compiling pipeline for Albedo ({:08b})", encoding.specialization);

					auto vertex = callable("main") << []() {
						layout_in <vec3> position(0);
						layout_in <vec2> uv(2);

						layout_out <vec2> out_uv(2);

						// Projection informations
						push_constant <MVP> mvp;
						gl_Position = mvp.project(position);
						gl_Position.y = -gl_Position.y;

						out_uv = uv;
					};
					
					auto fragment = callable("main") << std::make_tuple(texture) << [](bool texture) {
						layout_in <vec2> uv(2);

						layout_out <vec4> fragment(0);

						if (texture) {
							sampler2D albedo(0);

							fragment = albedo.sample(uv);
							cond(fragment.w < 0.1f);
								discard();
							end();

							fragment.w = 1.0f;
						} else {
							constexpr size_t mvp_size = sizeof(solid_t <MVP>);
							constexpr size_t vec3_size = sizeof(solid_t <vec3>);
							constexpr size_t aligned_mvp_size = ((mvp_size / vec3_size) + 1) * vec3_size;

							push_constant <vec3> albedo(aligned_mvp_size);

							fragment = vec4(albedo, 1.0);
						}
					};
					
					auto vertex_layout = littlevk::VertexLayout <
						littlevk::rgb32f,
						littlevk::rgb32f,
						littlevk::rg32f
					> ();
	
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
					} else {
						constexpr size_t mvp_size = sizeof(solid_t <MVP>);
						constexpr size_t vec3_size = sizeof(solid_t <vec3>);
						constexpr size_t aligned_mvp_size = ((mvp_size / vec3_size) + 1) * vec3_size;

						ppa.with_push_constant <solid_t <vec3>> (vk::ShaderStageFlagBits::eFragment, aligned_mvp_size);
					}

					pipelines[encoding] = ppa;

					// TODO: auto pipeline construction
				}

				if (texture) {
					// TODO: generate from compilation
					std::vector <vk::DescriptorSetLayoutBinding> binding {
						vk::DescriptorSetLayoutBinding {
							0, vk::DescriptorType::eCombinedImageSampler,
							1, vk::ShaderStageFlagBits::eFragment
						}
					};

					auto &ppl = pipelines[encoding];

					descriptors[material.uuid.global] = littlevk::bind(drc.device, drc.descriptor_pool)
						.allocate_descriptor_sets(*ppl.dsl).front();

					auto &descriptor = descriptors[material.uuid.global];

					auto sampler = littlevk::SamplerAssembler(drc.device, drc.dal);

					auto binder = littlevk::bind(drc.device, descriptor, binding);
					if (material.kd.is <vulkan::UnloadedTexture> ()) {
						auto &image = info.device_texture_bank["$checkerboard"];
						binder.queue_update(0, 0, sampler, image.view, vk::ImageLayout::eShaderReadOnlyOptimal);

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

					binder.finalize();
				}
			}
		}
	}

	if (viewport.mode == ViewportMode::eAlbedo) {
		for (size_t i = 0; i < scene.uuids.size(); i++) {
			auto &mesh = scene.meshes[i];
			mvp[m_id] = scene.uuids[i];

			// TODO: one mesh, one geometry

			auto mid = *mesh.material_usage.begin();
			auto &material = scene.materials[mid];

			bool texture = enabled(material.flags, vulkan::MaterialFlags::eAlbedoSampler);
			auto encoding = PipelineEncoding(ViewportMode::eAlbedo, texture);
			
			auto &ppl = pipelines[encoding];

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);
			
			cmd.pushConstants <solid_t <MVP>> (ppl.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);
			
			constexpr size_t mvp_size = sizeof(solid_t <MVP>);
			constexpr size_t vec3_size = sizeof(solid_t <vec3>);
			constexpr size_t aligned_mvp_size = ((mvp_size / vec3_size) + 1) * vec3_size;

			if (texture) {
				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
					ppl.layout, 0,
					descriptors.at(material.uuid.global), {});
			} else {
				cmd.pushConstants <float3> (ppl.layout,
					vk::ShaderStageFlagBits::eFragment,
					aligned_mvp_size,
					material.kd.as <float3> ());
			}
			
			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
		}
	} else {
		auto &ppl = pipelines[viewport.mode];

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

		for (size_t i = 0; i < scene.uuids.size(); i++) {
			auto &mesh = scene.meshes[i];
			mvp[m_id] = scene.uuids[i];

			cmd.pushConstants <solid_t <MVP>> (ppl.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);

			cmd.pushConstants <solid_t <u32>> (ppl.layout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(solid_t <MVP>),
				viewport.selected == scene.uuids[i]);

			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
		}
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