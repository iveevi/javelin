#include "common/aperature.hpp"
#include "common/camera_controller.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/vulkan_resources.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/transform.hpp"
#include "common/vulkan_triangle_mesh.hpp"
#include "common/application.hpp"

#include "shaders.hpp"

// Constructing the graphics pipeline
littlevk::Pipeline configure_pipeline(VulkanResources &drc,
				      const vk::RenderPass &render_pass)
{
	auto [binding, attributes] = binding_and_attributes(VertexFlags::ePosition);

	auto vs_callable = procedure("main") << vertex;
	auto fs_callable = procedure("main") << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	// TODO: automatic generation by observing used layouts
	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	return littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_binding(binding)
		.with_vertex_attributes(attributes)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
}

struct Application : BaseApplication {
	littlevk::Pipeline raster;
	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;
	
	Transform camera_transform;
	Transform model_transform;
	Aperature aperature;

	CameraController controller;

	std::vector <VulkanTriangleMesh> meshes;

	Application() : BaseApplication("Normals", { VK_KHR_SWAPCHAIN_EXTENSION_NAME }),
			controller(camera_transform, CameraControllerSettings())
	{
		render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
			.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
			.add_attachment(littlevk::default_depth_attachment())
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.done();

		// Configure ImGui
		configure_imgui(resources, render_pass);

		// Configure pipeline
		raster = configure_pipeline(resources, render_pass);
		
		// Framebuffer manager
		framebuffers.resize(resources, render_pass);
	}

	void configure(argparse::ArgumentParser &program) override {
		program.add_argument("mesh")
			.help("input mesh");
	}

	void preload(const argparse::ArgumentParser &program) override {
		// Load the asset and scene
		std::filesystem::path path = program.get("mesh");
	
		auto asset = ImportedAsset::from(path).value();

		for (auto &g : asset.geometries) {
			auto m = TriangleMesh::from(g).value();
			auto v = VulkanTriangleMesh::from(resources.allocator(), m, VertexFlags::ePosition).value();
			meshes.emplace_back(v);
		}
	}
	
	void render(const vk::CommandBuffer &cmd, uint32_t index) override {
		controller.handle_movement(resources.window);
		
		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(resources.window.extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[index])
			.with_extent(resources.window.extent)
			.clear_color(0, std::array <float, 4> { 1, 1, 1, 1 })
			.clear_depth(1, 1)
			.begin(cmd);
	
		// MVP structure used for push constants
		auto m_model = uniform_field(MVP, model);
		auto m_view = uniform_field(MVP, view);
		auto m_proj = uniform_field(MVP, proj);
		
		solid_t <MVP> mvp;

		mvp[m_model] = model_transform.matrix();
		
		auto &extent = resources.window.extent;
		aperature.aspect = float(extent.width)/float(extent.height);

		mvp[m_proj] = aperature.perspective();
		mvp[m_view] = camera_transform.view_matrix();
		
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, raster.handle);
	
		for (auto &mesh : meshes) {
			cmd.pushConstants <solid_t <MVP>> (raster.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);

			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
		}
		
		cmd.endRenderPass();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
	
	// Mouse button callbacks
	void mouse_inactive() override {
		controller.voided = true;
		controller.dragging = false;
	}
	
	void mouse_left_press() override {
		controller.dragging = true;
	}
	
	void mouse_left_release() override {
		controller.voided = true;
		controller.dragging = false;
	}

	void mouse_cursor(const glm::vec2 &xy) override {
		controller.handle_cursor(xy);
	}
};

APPLICATION_MAIN()