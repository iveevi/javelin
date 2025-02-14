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

struct Application : CameraApplication {
	littlevk::Pipeline traditional;
	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;
	
	Transform model_transform;

	float saturation = 0.5f;
	float lightness = 1.0f;
	int splits = 16;

	std::vector <VulkanTriangleMesh> meshes;

	Application() : CameraApplication("Palette", { VK_KHR_SWAPCHAIN_EXTENSION_NAME })
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
		
		// Framebuffer manager
		framebuffers.resize(resources, render_pass);
	}

	void compile_pipeline(VertexFlags flags, float saturation, float lightness, int splits) {
		auto vs_callable = procedure("main")
			<< vertex;

		auto fs_callable = procedure("main")
			<< std::make_tuple(saturation, lightness, splits)
			<< fragment;

		std::string vertex_shader = link(vs_callable).generate_glsl();
		std::string fragment_shader = link(fs_callable).generate_glsl();

		fmt::println("{}", vertex_shader);
		fmt::println("{}", fragment_shader);

		// TODO: automatic generation by observing used layouts
		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

		auto [binding, attributes] = binding_and_attributes(flags);

		traditional = littlevk::PipelineAssembler <littlevk::eGraphics> (resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_vertex_binding(binding)
			.with_vertex_attributes(attributes)
			.with_shader_bundle(bundle)
			.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
			.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
			.cull_mode(vk::CullModeFlagBits::eNone);
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

		compile_pipeline(meshes[0].flags, saturation, lightness, splits);
	}
	
	void render(const vk::CommandBuffer &cmd, uint32_t index) override {
		camera.controller.handle_movement(resources.window);
		
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
		camera.aperature.aspect = float(extent.width)/float(extent.height);

		mvp[m_proj] = camera.aperature.perspective();
		mvp[m_view] = camera.transform.view_matrix();
		
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, traditional.handle);
	
		for (auto &mesh : meshes) {
			cmd.pushConstants <solid_t <MVP>> (traditional.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);

			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
		}

		// ImGui window to configure the palette
		{
			ImGuiRenderContext context(cmd);

			// TODO: preview the colors in a grid
			ImGui::Begin("Configure Palette");

			ImGui::SliderFloat("saturation", &saturation, 0, 1);
			ImGui::SliderFloat("lightness", &lightness, 0, 1);
			ImGui::SliderInt("splits", &splits, 4, 64);
			if (ImGui::Button("Confirm"))
				compile_pipeline(meshes[0].flags, saturation, lightness, splits);

			ImGui::End();
		}
		
		cmd.endRenderPass();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()