#include "common/imported_asset.hpp"
#include "common/application.hpp"
#include "common/imgui.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/aperature.hpp"
#include "common/transform.hpp"
#include "common/camera_controller.hpp"
#include "common/triangle_mesh.hpp"

#include "shaders.hpp"

MODULE(meshlet);

// TODO: meshlet constructions modes
// - consecutive
// - neighboring cluster
// - meshlet frustrum culling

struct Application : CameraApplication {
	littlevk::Pipeline meshlet;

	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;
	
	Transform model_transform;

	littlevk::Buffer meshlet_triangles;
	littlevk::Buffer meshlet_vertices;
	littlevk::Buffer meshlet_sizes;
	uint32_t meshlet_count;

	vk::DescriptorSet descriptor;

	static inline const std::vector <vk::DescriptorSetLayoutBinding> meshlet_bindings {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eTaskEXT),
		vk::DescriptorSetLayoutBinding()
			.setBinding(1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eMeshEXT),
		vk::DescriptorSetLayoutBinding()
			.setBinding(2)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eTaskEXT),
	};

	Application() : CameraApplication("Meshlet", {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_MESH_SHADER_EXTENSION_NAME,
			}, features_include, features_activate)
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

		// Compile the pipeline
		compile_meshlet_pipeline();
	}

	static void features_include(VulkanFeatureChain &features) {
		features.add <vk::PhysicalDeviceMeshShaderFeaturesEXT> ();
		features.add <vk::PhysicalDeviceMaintenance4FeaturesKHR> ();
	}

	static void features_activate(VulkanFeatureChain &features) {
		for (auto &ptr : features) {
			switch (ptr->sType) {
			feature_case(vk::PhysicalDeviceMeshShaderFeaturesEXT)
				.setMeshShader(true)
				.setTaskShader(true)
				.setMeshShaderQueries(false)
				.setPrimitiveFragmentShadingRateMeshShader(false)
				.setMultiviewMeshShader(false);
				break;
			feature_case(vk::PhysicalDeviceMaintenance4Features)
				.setMaintenance4(true);
				break;
			default:
				break;
			}
		}
	}

	void compile_meshlet_pipeline() {
		thunder::opt_transform(task);
		thunder::opt_transform(mesh);

		std::string task_shader = link(task).generate_glsl();
		std::string mesh_shader = link(mesh).generate_glsl();
		std::string fragment_shader = link(fragment).generate_glsl();

		fmt::println("{}", task_shader);
		fmt::println("{}", mesh_shader);

		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(task_shader, vk::ShaderStageFlagBits::eTaskEXT)
			.source(mesh_shader, vk::ShaderStageFlagBits::eMeshEXT)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

		meshlet = littlevk::PipelineAssembler <littlevk::eGraphics> (resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(bundle)
			.with_push_constant <solid_t <ViewInfo>> (vk::ShaderStageFlagBits::eMeshEXT, 0)
			.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eTaskEXT)
			.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eMeshEXT)
			.with_dsl_binding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eMeshEXT)
			.cull_mode(vk::CullModeFlagBits::eNone);

		descriptor = littlevk::bind(resources.device, resources.descriptor_pool)
			.allocate_descriptor_sets(meshlet.dsl.value())
			.front();
	}

	void configure(argparse::ArgumentParser &program) override {
		program.add_argument("mesh")
			.help("input mesh");
	}

	void preload(const argparse::ArgumentParser &program) override {
		// Load the asset and scene
		std::filesystem::path path = program.get("mesh");
	
		auto asset = ImportedAsset::from(path).value();

		// TODO: connected meshlets
		auto &g = asset.geometries[0];
		auto tm = TriangleMesh::from(g).value();

		// TODO: scalar format for buffer
		// std::vector <glm::vec3> vertices;
		// std::vector <glm::ivec3> triangles;
		std::vector <glm::vec4> vertices;
		std::vector <glm::ivec4> triangles;
		std::vector <uint32_t> sizes(1);

		uint32_t offset = 0;

		for (auto &tri : tm.triangles) {
			// Pump to continguous buffers
			vertices.emplace_back(tm.positions[tri.x], 0);
			vertices.emplace_back(tm.positions[tri.y], 0);
			vertices.emplace_back(tm.positions[tri.z], 0);
			
			triangles.emplace_back(offset, offset + 1, offset + 2, 0);
			sizes.back()++;

			if (triangles.size() % MESHLET_SIZE == 0) {
				sizes.emplace_back(0);
				offset = 0;
			} else {
				offset += 3;
			}
		}

		// Upload contiguous buffers
		std::tie(meshlet_vertices, meshlet_triangles, meshlet_sizes) = resources.allocator()
			.buffer(vertices, vk::BufferUsageFlagBits::eStorageBuffer)
			.buffer(triangles, vk::BufferUsageFlagBits::eStorageBuffer)
			.buffer(sizes, vk::BufferUsageFlagBits::eStorageBuffer);

		meshlet_count = sizes.size();

		// Bind descriptor resources
		littlevk::bind(resources.device, descriptor, meshlet_bindings)
			.queue_update(0, 0, meshlet_sizes.buffer, 0, vk::WholeSize)
			.queue_update(1, 0, meshlet_vertices.buffer, 0, vk::WholeSize)
			.queue_update(2, 0, meshlet_triangles.buffer, 0, vk::WholeSize)
			.finalize();
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
		auto &extent = resources.window.extent;
		
		camera.aperature.aspect = float(extent.width)/float(extent.height);

		auto m_model = uniform_field(ViewInfo, model);
		auto m_view = uniform_field(ViewInfo, view);
		auto m_proj = uniform_field(ViewInfo, proj);
		
		solid_t <ViewInfo> view_info;

		view_info[m_model] = model_transform.matrix();
		view_info[m_proj] = camera.aperature.perspective();
		view_info[m_view] = camera.transform.view_matrix();

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, meshlet.handle);
		cmd.pushConstants <solid_t <ViewInfo>> (meshlet.layout, vk::ShaderStageFlagBits::eMeshEXT, 0, view_info);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, meshlet.layout, 0, descriptor, {});
		// TODO: slider for the number of meshlets to draw...
		// TODO: also for the meshlet size...
		cmd.drawMeshTasksEXT(meshlet_count, 1, 1);
		
		cmd.endRenderPass();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()