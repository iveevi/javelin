#include <argparse/argparse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/transform.hpp"
#include "common/vulkan_resources.hpp"

#include "app.hpp"

Application::Application() : CameraApplication("Colored", { VK_KHR_SWAPCHAIN_EXTENSION_NAME })
{
	render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
		.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	// Framebuffer manager
	framebuffers.resize(resources, render_pass);

	// Configure ImGui
	configure_imgui(resources, render_pass);

	// Configure pipeline
	color = glm::vec3(1, 0, 0);

	compile_pipeline(color);
	shader_debug();
}

Application::~Application() {
	shutdown_imgui();
}

void Application::configure(argparse::ArgumentParser &program)
{
	program.add_argument("mesh")
		.help("input mesh");
}

void Application::preload(const argparse::ArgumentParser &program)
{
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

void Application::render(const vk::CommandBuffer &cmd, uint32_t index, uint32_t total)
{
	if (automatic) {
		float time = 0.01 * total;

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

void Application::resize()
{
	framebuffers.resize(resources, render_pass);
}

APPLICATION_MAIN()
