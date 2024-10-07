#include <core/color.hpp>
#include <thunder/opt.hpp>

#include "viewport_render_group.hpp"
#include "viewport.hpp"

// MODULE(viewport-render-group);

// Function to generate an HSV color palette in shader code
template <thunder::index_t N>
array <vec3> hsv_palette(float saturation, float lightness)
{
	std::array <vec3, N> palette;

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		float3 hsv = float3(i * step, saturation, lightness);
		float3 rgb = hsl_to_rgb(hsv);
		palette[i] = vec3(rgb.x, rgb.y, rgb.z);
	}

	return palette;
}

// Model-view-projection structure
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;
	i32 id;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj),
			named_field(id)
		);
	}
};

// Material information
struct UberMaterial {
	vec3 kd;
	vec3 ks;
	f32 roughness;

	auto layout() const {
		return uniform_layout(
			"Material",
			named_field(kd),
			named_field(ks),
			named_field(roughness)
		);
	}
};

// Vertex shader(s)
void vertex(ViewportMode mode)
{
	// Vertex inputs
	layout_in <vec3> position(0);
	layout_in <vec3> normal(1);
	layout_in <vec2> uv(2);

	// Regurgitate vertex positions
	layout_out <vec3> out_position(0);

	// TODO: allow duplicate bindings...
	layout_out <vec2> out_uv(2);

	// Object vertex ID
	layout_out <uint32_t, flat> out_id(0);

	// Object UUID
	layout_out <int32_t, flat> out_uuid(1);
	
	// Projection informations
	push_constant <MVP> mvp;
	gl_Position = mvp.project(position);
	gl_Position.y = -gl_Position.y;

	switch (mode) {
	case eNormal:
		out_position = position;
		break;
	case eTextureCoordinates:
		out_uv = uv;
		break;
	case eTriangles:
		out_id = u32(gl_VertexIndex / 3);
		break;
	default:
		break;
	}

	out_uuid = mvp.id;
}

// Fragment shader(s)
void fragment(ViewportMode mode)
{
	// Position from vertex shader
	layout_in <vec3> position(0);
	layout_in <vec2> uv(2);

	// Object vertex ID
	layout_in <uint32_t, flat> id(0);

	// Material properties
	uniform <UberMaterial> material(0);

	// Resulting fragment color
	layout_out <vec4> fragment(0);

	switch (mode) {

	case eAlbedo:
	{
		fragment = vec4(material.kd, 1.0);
	} break;
	
	case eNormal:
	{
		vec3 dU = dFdxFine(position);
		vec3 dV = dFdyFine(position);
		vec3 N = normalize(cross(dV, dU));
		fragment = vec4(0.5f + 0.5f * N, 1.0f);
	} break;

	case eTextureCoordinates:
	{
		fragment = vec4(uv.x, uv.y, 0.0, 1.0f);
	} break;

	case eTriangles:
	{
		auto palette = hsv_palette <16> (0.5, 1);

		fragment = vec4(palette[id % palette.length], 1);
	} break;

	case eDepth:
	{
		float near = 0.1f;
		float far = 10000.0f;
		
		f32 d = gl_FragCoord.z;
		f32 linearized = (near * far) / (far + d * (near - far));

		linearized = ire::log(linearized/250.0f + 1);

		fragment = vec4(vec3(linearized), 1);
	} break;

	default:
		fragment = vec4(1, 0, 1, 1);
		break;
	}

	// Highlighting the selected object
	push_constant <uint> highlight(sizeof(solid_t <MVP>));

	cond(highlight == 1u);
		fragment = mix(fragment, vec4(1, 1, 0, 1), 0.8f);
	end();
}

// Constructor	
ViewportRenderGroup::ViewportRenderGroup(DeviceResourceCollection &drc)
{
	configure_render_pass(drc);
	configure_pipelines(drc);
}

// Configuration
static const std::array <vk::DescriptorSetLayoutBinding, 1> albedo_bindings {
	vk::DescriptorSetLayoutBinding {
		0, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eFragment
	}
};

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

	auto ppa = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
		
	if (mode == eAlbedo)
		pipelines[mode] = ppa.with_dsl_bindings(albedo_bindings);
	else
		pipelines[mode] = ppa;
}

void ViewportRenderGroup::configure_pipelines(DeviceResourceCollection &drc)
{
	for (int32_t i = 0; i < eCount; i++) {
		if (i != eAlbedo)
			configure_pipeline_mode(drc, (ViewportMode) i);
	}
}

#include <thread>

struct LoadingWork {
	vulkan::Material &material;
	vk::DescriptorSet &descriptor;
	TextureBank &host_bank;
	vulkan::TextureBank &device_bank;
	DeviceResourceCollection &drc;
	wrapped::thread_safe_queue <vk::CommandBuffer> &extra;
	littlevk::Pipeline &ppl;
};

vk::CommandPool texture_loading_pool;
wrapped::thread_safe_queue <LoadingWork> work;
wrapped::thread_safe_queue <LoadingWork> pending;

std::thread *texture_loader = nullptr;

void texture_loader_worker()
{
	while (true) {
		if (!work.size()) {
			// TODO: sleep
			continue;
		}

		auto unit = work.pop();

		auto &kd = unit.material.kd;
		auto &drc = unit.drc;

		if (auto unloaded = kd.get <vulkan::UnloadedTexture> ()) {
			auto &texture = core::Texture::from(unit.host_bank, unloaded->path);

			unit.device_bank.upload(drc.allocator(),
				littlevk::bind(drc.device, texture_loading_pool, drc.graphics_queue),
				unloaded->path,
				texture,
				unit.extra);

			kd = vulkan::ReadyTexture(unloaded->path);

			pending.push(unit);
		} else if (auto ready = kd.get <vulkan::ReadyTexture> ()) {
			std::vector <vk::DescriptorSetLayoutBinding> binding {
				vk::DescriptorSetLayoutBinding {
					0, vk::DescriptorType::eCombinedImageSampler,
					1, vk::ShaderStageFlagBits::eFragment
				}
			};

			// TODO: destroy old descriptor set
			unit.descriptor = littlevk::bind(drc.device, drc.descriptor_pool)
				.allocate_descriptor_sets(*unit.ppl.dsl).front();
			
			auto binder = littlevk::bind(drc.device, unit.descriptor, binding);
			
			auto sampler = littlevk::SamplerAssembler(drc.device, drc.dal);

			auto &image = unit.device_bank[ready->path];
			binder.queue_update(0, 0, sampler,
				image.view,
				vk::ImageLayout::eShaderReadOnlyOptimal);
			binder.finalize();
		}
	}
}

// Rendering
void ViewportRenderGroup::render(const RenderingInfo &info, Viewport &viewport)
{
	auto &cmd = info.cmd;
	auto &drc = info.drc;
	auto &scene = info.device_scene;

	// Configure the rendering extent
	littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(viewport.extent));

	viewport.handle_input(info.window);

	littlevk::RenderPassBeginInfo(2)
		.with_render_pass(render_pass)
		.with_framebuffer(viewport.framebuffers[info.operation.index])
		.with_extent(viewport.extent)
		.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
		.clear_depth(1, 1)
		.begin(cmd);

	solid_t <MVP> mvp;

	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);
	auto m_id = uniform_field(MVP, id);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(viewport.aperature);
	mvp[m_view] = viewport.transform.to_view_matrix();

	if (viewport.mode == eAlbedo) {
		if (!texture_loader) {
			texture_loader = new std::thread(texture_loader_worker);

			auto queue_family = littlevk::find_queue_families(drc.phdev, drc.surface);

			texture_loading_pool = littlevk::command_pool(drc.device,
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				queue_family.graphics).unwrap(drc.dal);
		}

		for (size_t i = 0; i < scene.uuids.size(); i++) {
			auto &mesh = scene.meshes[i];
			for (auto i : mesh.material_usage) {
				// TODO: may need multiple descriptors per material
				auto &material = scene.materials[i];
				if (descriptors.contains(material.uuid.global))
					continue;

				// This only depends on the diffuse texture
				bool texture = enabled(material.flags, vulkan::MaterialFlags::eAlbedoSampler);

				PipelineEncoding encoding(eAlbedo, texture);

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

	if (viewport.mode == eAlbedo) {
		for (size_t i = 0; i < scene.uuids.size(); i++) {
			auto &mesh = scene.meshes[i];
			mvp[m_id] = scene.uuids[i];

			// TODO: one mesh, one geometry

			auto mid = *mesh.material_usage.begin();
			auto &material = scene.materials[mid];

			bool texture = enabled(material.flags, vulkan::MaterialFlags::eAlbedoSampler);
			auto encoding = PipelineEncoding(eAlbedo, texture);
			
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

	// Transfer all pending work to the current work
	while (pending.size()) {
		auto cmd = pending.pop();
		work.push(cmd);
		// TODO: push to the front so that already processed items are prioritized
	}
}