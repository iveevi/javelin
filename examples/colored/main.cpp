#include <argparse/argparse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "common/aperature.hpp"
#include "common/application.hpp"
#include "common/camera_controller.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/transform.hpp"
#include "common/vulkan_resources.hpp"
#include "common/vulkan_triangle_mesh.hpp"

#include "shaders.hpp"

struct Application : CameraApplication {
	littlevk::Pipeline raster;
	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;

	Transform model_transform;

	glm::vec3 color;
	glm::vec3 min;
	glm::vec3 max;
	bool automatic;

	std::vector <VulkanTriangleMesh> meshes;

	Application() : CameraApplication("Colored", { VK_KHR_SWAPCHAIN_EXTENSION_NAME })
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
		color = glm::vec3(1, 0, 0);

		compile_pipeline(color);
		
		// Framebuffer manager
		framebuffers.resize(resources, render_pass);
	}

	void compile_pipeline(const glm::vec3 &color) {
		auto vs_callable = procedure("main") << vertex;
		auto fs_callable = procedure("main") << std::make_tuple(color) << fragment;

		std::string vertex_shader = link(vs_callable).generate_glsl();
		std::string fragment_shader = link(fs_callable).generate_glsl();

		fmt::println("{}", vertex_shader);
		fmt::println("{}", fragment_shader);

		// TODO: automatic generation by observing used layouts
		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);
		
		auto [binding, attributes] = binding_and_attributes(VertexFlags::ePosition);

		raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
			(resources.device, resources.window, resources.dal)
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
		
		min = glm::vec3(1e10);
		max = -min;

		for (auto &g : asset.geometries) {
			auto m = TriangleMesh::from(g).value();
			auto v = VulkanTriangleMesh::from(resources.allocator(), m, VertexFlags::ePosition).value();
			meshes.emplace_back(v);

			auto [bmin, bmax] = m.scale();

			min = glm::min(min, bmin);
			max = glm::max(max, bmax);
		}

		automatic = (program["auto"] == true);
	}

	void render(const vk::CommandBuffer &cmd, uint32_t index) override {
		if (automatic) {
			float time = 2.5 * glfwGetTime();

			auto &xform = camera.transform;

			float r = 0.75 * glm::length(max - min);
			float a = time + glm::pi <float> () / 2.0f;

			xform.translate = glm::vec3 {
				r * glm::cos(time),
				-0.5 * (max + min).y,
				r * glm::sin(time),
			};

			xform.rotation = glm::angleAxis(-a, glm::vec3(0, 1, 0));
		} else {
			camera.controller.handle_movement(resources.window);
		}
		
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
		solid_t <MVP> mvp;

		mvp.get <0> () = model_transform.matrix();
	
		auto &extent = resources.window.extent;
		camera.aperature.aspect = float(extent.width)/float(extent.height);

		mvp.get <1> () = camera.transform.view_matrix();
		mvp.get <2> () = camera.aperature.perspective();
		
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, raster.handle);
	
		for (auto &mesh : meshes) {
			cmd.pushConstants <solid_t <MVP>> (raster.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);

			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(3 * mesh.triangle_count, 1, 0, 0, 0);
		}

		// ImGui window to configure the palette 
		{
			ImGuiRenderContext context(cmd);

			ImGui::Begin("Configure Pipeline");
			
			ImGui::ColorEdit3("color", reinterpret_cast <float *> (&color));
			if (ImGui::Button("Confirm"))
				compile_pipeline(color);

			ImGui::End();
		}

		cmd.endRenderPass();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()