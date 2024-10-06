#include <core/color.hpp>
#include <thunder/opt.hpp>

#include "viewport_render_group.hpp"
#include "viewport.hpp"

MODULE(viewport-render-group);

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
	for (int32_t i = 0; i < eCount; i++)
		configure_pipeline_mode(drc, (ViewportMode) i);
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

	auto &ppl = pipelines[viewport.mode];

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

	solid_t <MVP> mvp;

	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);
	auto m_id = uniform_field(MVP, id);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(viewport.aperature);
	mvp[m_view] = viewport.transform.to_view_matrix();

	static std::map <uint64_t, vk::DescriptorSet> material_descriptors;

	if (viewport.mode == eAlbedo) {
		auto allocator = littlevk::bind(drc.device, drc.descriptor_pool);
		for (size_t i = 0; i < scene.uuids.size(); i++) {
			auto &mesh = scene.meshes[i];
			for (auto i : mesh.material_usage) {
				if (material_descriptors.contains(i))
					continue;

				auto &material = scene.materials[i];

				vk::DescriptorSet descriptor = allocator.allocate_descriptor_sets(*ppl.dsl).front();
				JVL_ASSERT_PLAIN(descriptor);

				littlevk::bind(drc.device, descriptor, albedo_bindings)
					.update(0, 0, *material.uber, 0, sizeof(vulkan::Material::uber_x))
					.finalize();

				material_descriptors[i] = descriptor;
			}
		}
	}

	for (size_t i = 0; i < scene.uuids.size(); i++) {
		auto &mesh = scene.meshes[i];
		mvp[m_id] = scene.uuids[i];

		int mid = *mesh.material_usage.begin();

		if (viewport.mode == eAlbedo) {
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				ppl.layout, 0,
				material_descriptors[mid], {});
		}

		cmd.pushConstants <solid_t <MVP>> (ppl.layout,
			vk::ShaderStageFlagBits::eVertex,
			0,
			mvp);

		cmd.pushConstants <solid_t <u32>> (ppl.layout,
			vk::ShaderStageFlagBits::eFragment,
			sizeof(solid_t <MVP>),
			viewport.selected == scene.uuids[i]);

		cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
		cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
	}

	cmd.endRenderPass();

	littlevk::transition(info.cmd, viewport.targets[info.operation.index],
		vk::ImageLayout::ePresentSrcKHR,
		vk::ImageLayout::eShaderReadOnlyOptimal);
}