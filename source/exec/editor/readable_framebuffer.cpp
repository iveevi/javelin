#include "shaders.hpp"
#include "readable_framebuffer.hpp"
	
MODULE(readable-framebuffer);
	
ReadableFramebuffer::ReadableFramebuffer(DeviceResourceCollection &drc,
					 const Viewport &viewport,
					 const vk::Format &format)
		: Unique(new_uuid <ReadableFramebuffer> ()),
		aperature(viewport.aperature),
		transform(viewport.transform),
		extent(viewport.extent)
{
	// TODO: constexpr function
	static const std::map <vk::Format, FormatInfo> format_infos {
		{ vk::Format::eR32Sint, { 4, 1 } }
	};

	JVL_ASSERT(format_infos.contains(format),
		"unsupported format for readable framebuffer: {}",
		vk::to_string(format));
	
	render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	auto info = format_infos.at(format);
	
	littlevk::Image depth;

	std::tie(image, depth, staging) = drc.allocator()
		.image(extent,
			format,
			vk::ImageUsageFlagBits::eColorAttachment
			| vk::ImageUsageFlagBits::eTransferSrc,
			vk::ImageAspectFlagBits::eColor)
		.image(extent,
			vk::Format::eD32Sfloat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth)
		.buffer(info.bytes(extent),
			vk::BufferUsageFlagBits::eTransferDst);

	littlevk::FramebufferGenerator generator {
		drc.device, render_pass,
		extent, drc.dal
	};

	generator.add(image.view, depth.view);

	framebuffer = generator.unpack().front();
}

// TODO: should use the viewport render group
void ReadableFramebuffer::configure_pipeline(DeviceResourceCollection &drc, const vulkan::VertexFlags &flags)
{
	if (pipelines.contains(flags))
		return;

	auto [binding, attributes] = vulkan::binding_and_attributes(flags);

	auto vs = procedure("main") << []() {
		push_constant <MVP> mvp;
		layout_in <vec3> position(0);
		gl_Position = mvp.project(position);
		gl_Position.y = -gl_Position.y;
	};

	auto fs = procedure("main") << []() {
		push_constant <i32> id(solid_size <MVP>);
		layout_out <i32> obj(0);
		obj = id;
	};

	std::string vertex_shader = link(vs).generate_glsl();
	std::string fragment_shader = link(fs).generate_glsl();

	auto shaders = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);
	
	pipelines[flags] = littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_shader_bundle(shaders)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, solid_size <MVP>)
		.cull_mode(vk::CullModeFlagBits::eNone)
		.alpha_blending(false)
		.with_vertex_binding(binding)
		.with_vertex_attributes(attributes);
}

void ReadableFramebuffer::apply_request(const ObjectSelection &r)
{
	request = r;
}

void ReadableFramebuffer::go(const RenderingInfo &info, Null)
{
	auto &cmd = info.cmd;
	auto &scene = info.device_scene;

	// Start the render pass
	littlevk::RenderPassBeginInfo(2)
		.with_render_pass(render_pass)
		.with_framebuffer(framebuffer)
		.with_extent(extent)
		.clear_color(0, std::array <int32_t, 4> { -1, -1, -1, -1 })
		.clear_depth(1, 1)
		.begin(cmd);
	
	littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(extent));

	// Common camera information
	solid_t <MVP> mvp;

	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);

	mvp[m_model] = jvl::float4x4::identity();
	mvp[m_proj] = jvl::core::perspective(aperature);
	mvp[m_view] = transform.to_view_matrix();

	// Render
	for (auto &mesh : scene.meshes) {
		configure_pipeline(info.drc, mesh.flags);

		JVL_ASSERT_PLAIN(pipelines.contains(mesh.flags));

		auto &pipeline = pipelines[mesh.flags];

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);

		cmd.pushConstants <solid_t <MVP>> (pipeline.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, mvp);

		cmd.pushConstants <solid_t <i32>> (pipeline.layout,
			vk::ShaderStageFlagBits::eFragment,
			sizeof(solid_t <MVP>),
			scene.mesh_to_object[mesh]);

		cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
		cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
	}

	cmd.endRenderPass();

	stage = WaitCopy();
}

void ReadableFramebuffer::go(const RenderingInfo &info, WaitCopy)
{
	image.transition(info.cmd, vk::ImageLayout::eTransferSrcOptimal);

	littlevk::copy_image_to_buffer(info.cmd,
		image,
		staging,
		vk::ImageLayout::eTransferSrcOptimal);

	stage = WaitRead();
}

void ReadableFramebuffer::go(const RenderingInfo &info, WaitRead)
{
	std::vector <int32_t> ids(extent.width * extent.height);
	littlevk::download(info.drc.device, staging, ids);
	size_t index = request.index(extent);
	fmt::println("id at index: {}", ids[index]);

	auto update_message = message(editor_update_selected_object, int64_t(ids[index]));
	auto removal_message = message(editor_remove_self);

	info.message_system.send_to_origin(update_message);
	info.message_system.send_to_origin(removal_message);
}

void ReadableFramebuffer::go(const RenderingInfo &info)
{
	auto ftn = [&](auto x) { return go(info, x); };
	return std::visit(ftn, stage);
}