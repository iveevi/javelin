#include <argparse/argparse.hpp>

#include "common/aperature.hpp"
#include "common/application.hpp"
#include "common/camera_controller.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/imgui.hpp"
#include "common/transform.hpp"

#include "ngf.hpp"
#include "shaders.hpp"

MODULE(ngf);

constexpr vk::DescriptorSetLayoutBinding texture_at(uint32_t binding,
						   vk::ShaderStageFlagBits extra = vk::ShaderStageFlagBits::eMeshEXT)
{
	return vk::DescriptorSetLayoutBinding {
		binding, vk::DescriptorType::eCombinedImageSampler,
		1, vk::ShaderStageFlagBits::eMeshEXT | extra
	};
}

const std::vector <vk::DescriptorSetLayoutBinding> meshlet_bindings {
	// Complexes
	texture_at(0, vk::ShaderStageFlagBits::eTaskEXT),

	// Vertices
	texture_at(1, vk::ShaderStageFlagBits::eTaskEXT),

	// Features
	texture_at(2),

	// Bias vectors
	texture_at(3),

	// Layer weights
	texture_at(4),
	texture_at(5),
	texture_at(6),
	texture_at(7),
};

template <typename T>
littlevk::Image general_allocator(const vk::Device &device,
				  const vk::PhysicalDeviceMemoryProperties &memory_properties,
				  const vk::CommandPool &command_pool,
				  const vk::Queue &graphics_queue,
				  littlevk::Deallocator &dal,
				  const std::vector <T> &buffer,
				  const vk::Extent2D &extent,
				  const vk::ImageType type = vk::ImageType::e2D,
				  const vk::Format format = vk::Format::eR32G32B32A32Sfloat)
{
	vk::ImageViewType view = (type == vk::ImageType::e2D)
		? vk::ImageViewType::e2D
		: vk::ImageViewType::e1D;

	littlevk::Image texture;
	littlevk::Buffer staging;

	fmt::println("allocating texture with extent={}x{}, bytes={} ({})", extent.width, extent.height, extent.width * extent.height * sizeof(glm::vec4), sizeof(glm::vec4));
	fmt::println("buffer.size() = {}, bytes={} ({})", buffer.size(), buffer.size() * sizeof(T), sizeof(T));

	std::tie(texture, staging) = bind(device, memory_properties, dal)
		.image(extent, format,
			vk::ImageUsageFlagBits::eSampled
			| vk::ImageUsageFlagBits::eTransferDst,
			vk::ImageAspectFlagBits::eColor,
			type,
			view)
		.buffer(buffer, vk::BufferUsageFlagBits::eTransferSrc);

	littlevk::submit_now(device, command_pool, graphics_queue,
		[&](const vk::CommandBuffer &cmd) {
			littlevk::transition(cmd, texture,
					vk::ImageLayout::eUndefined,
					vk::ImageLayout::eTransferDstOptimal);

			littlevk::copy_buffer_to_image(cmd, texture, staging,
					vk::ImageLayout::eTransferDstOptimal);

			littlevk::transition(cmd, texture,
					vk::ImageLayout::eTransferDstOptimal,
					vk::ImageLayout::eShaderReadOnlyOptimal);
		}
	);

	return texture;
}

template <typename T>
static const auto allocator(VulkanResources &resources,
			    const std::vector <T> &buffer,
			    const vk::Extent2D &extent,
			    const vk::ImageType &type = vk::ImageType::e2D,
			    const vk::Format &format = vk::Format::eR32G32B32A32Sfloat)
{
	return general_allocator(resources.device,
		resources.memory_properties,
		resources.command_pool,
		resources.graphics_queue,
		resources.dal,
		buffer,
		extent,
		type,
		format);
};

struct Application : CameraApplication {
	littlevk::Pipeline traditional;
	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;
	
	Transform model_transform;

	ImportedNGF ingf;
	HomogenizedNGF hngf;

	vk::DescriptorSet descriptor;

	Application() : CameraApplication("Neural Geometry Fields", {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_MESH_SHADER_EXTENSION_NAME,
				VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
				VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
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
	}

	// Feature configuration
	static void features_include(VulkanFeatureChain &features) {
		features.add <vk::PhysicalDeviceMaintenance4FeaturesKHR> ();
		features.add <vk::PhysicalDeviceMeshShaderFeaturesEXT> ();
	}

	static void features_activate(VulkanFeatureChain &features) {
		for (auto &ptr : features) {
			switch (ptr->sType) {
			feature_case(vk::PhysicalDeviceMaintenance4FeaturesKHR)
				.setMaintenance4(true);
				break;
			feature_case(vk::PhysicalDeviceMeshShaderFeaturesEXT)
				.setMeshShader(true)
				.setTaskShader(true)
				.setMultiviewMeshShader(false)
				.setPrimitiveFragmentShadingRateMeshShader(false)
				.setMeshShaderQueries(false);
				break;
			default:
				break;
			}
		}
	}

	// Program arguments
	void configure(argparse::ArgumentParser &program) override {
		program.add_argument("binary")
			.help("neural geometry field binary file");
	}

	void preload(const argparse::ArgumentParser &program) override {
		// Load the neural geometry field itself
		std::filesystem::path path = program.get("binary");

		if (!std::filesystem::exists(path))
			JVL_ABORT("failed to load neural geometry field: {}", path.string());

		ingf = ImportedNGF::load(path);
		hngf = HomogenizedNGF::from(ingf);

		// Compile appropriate pipeline
		// TODO: fix bugs with unoptimized version (variables instantiated inside block)
		thunder::opt_transform(task);
		thunder::opt_transform(mesh);

		task.dump();
		mesh.dump();

		std::string task_shader = link(task).generate_glsl();
		std::string mesh_shader = link(mesh).generate_glsl();
		std::string fragment_shader = link(fragment).generate_glsl();

		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(task_shader, vk::ShaderStageFlagBits::eTaskEXT)
			.source(mesh_shader, vk::ShaderStageFlagBits::eMeshEXT)
			.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

		traditional = littlevk::PipelineAssembler <littlevk::eGraphics> (resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(bundle)
			.with_dsl_bindings(meshlet_bindings)
			.with_push_constant <solid_t <ViewInfo>> (vk::ShaderStageFlagBits::eMeshEXT
								  | vk::ShaderStageFlagBits::eTaskEXT);

		// Allocate textures for the NGF
		uint32_t patches = hngf.patches.size();
		uint32_t features = hngf.features.size() / patches;
		uint32_t vertices = hngf.vertices.size();
		uint32_t biases = hngf.biases.size();
		uint32_t wsize = hngf.W0.size() >> 6;

		JVL_INFO("neural geometry field with patches={}, vertices={}, features={}, biases={}, weights={}",
			patches, vertices, features, biases, wsize);

		JVL_INFO("features.size() = {}", hngf.features.size());

		auto complex_texture = allocator(resources, hngf.patches,
			{ patches, 1 },
			vk::ImageType::e1D,
			vk::Format::eR32G32B32A32Sint);

		auto vertex_texture = allocator(resources, hngf.vertices, { vertices, 1 }, vk::ImageType::e1D);
		auto feature_texture = allocator(resources, hngf.features, { features, patches });

		auto bias_texture = allocator(resources, hngf.biases, { biases / 4, 1 }, vk::ImageType::e1D);
		auto W0_texture = allocator(resources, hngf.W0, { 16, wsize });
		auto W1_texture = allocator(resources, hngf.W1, { 16, 64 });
		auto W2_texture = allocator(resources, hngf.W2, { 16, 64 });
		auto W3_texture = allocator(resources, hngf.W3, { 16, 3 });

		// Bind the resources
		descriptor = littlevk::bind(resources.device, resources.descriptor_pool)
			.allocate_descriptor_sets(traditional.dsl.value()).front();

		vk::Sampler integer_sampler = littlevk::SamplerAssembler(resources.device, resources.dal)
			.filtering(vk::Filter::eNearest)
			.mipping(vk::SamplerMipmapMode::eNearest);
		
		vk::Sampler floating_sampler = littlevk::SamplerAssembler(resources.device, resources.dal);

		auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;

		littlevk::bind(resources.device, descriptor, meshlet_bindings)
			.queue_update(0, 0, integer_sampler, complex_texture.view, layout)
			.queue_update(1, 0, floating_sampler, vertex_texture.view, layout)
			.queue_update(2, 0, floating_sampler, feature_texture.view, layout)
			.queue_update(3, 0, floating_sampler, bias_texture.view, layout)
			.queue_update(4, 0, floating_sampler, W0_texture.view, layout)
			.queue_update(5, 0, floating_sampler, W1_texture.view, layout)
			.queue_update(6, 0, floating_sampler, W2_texture.view, layout)
			.queue_update(7, 0, floating_sampler, W3_texture.view, layout)
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
		
		auto m_model = uniform_field(ViewInfo, model);
		auto m_view = uniform_field(ViewInfo, view);
		auto m_proj = uniform_field(ViewInfo, proj);
		auto m_resl = uniform_field(ViewInfo, resolution);
		
		solid_t <ViewInfo> view_info;

		view_info[m_model] = model_transform.matrix();
		view_info[m_proj] = camera.aperature.perspective();
		view_info[m_view] = camera.transform.view_matrix();
		view_info[m_resl] = 2;

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, traditional.handle);

		cmd.pushConstants <solid_t <ViewInfo>> (traditional.layout,
			vk::ShaderStageFlagBits::eTaskEXT
			| vk::ShaderStageFlagBits::eMeshEXT,
			0, view_info);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, traditional.layout, 0, descriptor, { });
		// cmd.drawMeshTasksEXT(ingf.patch_count, 1, 1);
		cmd.drawMeshTasksEXT(1, 1, 1);
		
		cmd.endRenderPass();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()