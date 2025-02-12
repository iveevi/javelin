#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <argparse/argparse.hpp>

#include <ire/core.hpp>

#include "aperature.hpp"
#include "camera_controller.hpp"
#include "default_framebuffer_set.hpp"
#include "vulkan_resources.hpp"
#include "imgui.hpp"
#include "imported_asset.hpp"
#include "timer.hpp"
#include "transform.hpp"
#include "vulkan_triangle_mesh.hpp"
#include "application.hpp"

using namespace jvl;
using namespace jvl::ire;

// Model-view-projection structure
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj)
		);
	}
};

// Shader kernels
void vertex()
{
	// Vertex inputs
	layout_in <vec3> position(0);
	
	// Stage outputs
	layout_out <vec3> out_position(0);
	
	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);

	// Regurgitate positions
	out_position = position;
}

void fragment()
{
	// Triangle index
	layout_in <vec3> position(0);
	
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	// Render the normal vectors
	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));
	fragment = vec4(0.5f + 0.5f * N, 1.0f);
}

// Constructing the graphics pipeline
littlevk::Pipeline configure_pipeline(VulkanResources &drc,
				      const vk::RenderPass &render_pass)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

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
		.with_vertex_layout(vertex_layout)
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

		auto handle = resources.window.handle;
		glfwSetWindowUserPointer(handle, &controller);
		glfwSetMouseButtonCallback(handle, glfw_button_callback);
		glfwSetCursorPosCallback(handle, glfw_cursor_callback);
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
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 0 })
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