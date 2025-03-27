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

	vk::DescriptorSet rtx_descriptor;

	int32_t resolution;
	glm::vec3 min;
	glm::vec3 max;
	bool automatic;

	Application() : CameraApplication("Neural Geometry Fields", {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_MESH_SHADER_EXTENSION_NAME,
				VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
				VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
			}, features_include, features_activate),
			resolution(4)
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

	~Application() {
		shutdown_imgui();
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

		program.add_argument("--file")
			.help("use shader source file")
			.flag();
	}

	void preload(const argparse::ArgumentParser &program) override {
		// Load the neural geometry field itself
		std::filesystem::path path = program.get("binary");

		if (!std::filesystem::exists(path))
			JVL_ABORT("failed to load neural geometry field: {}", path.string());

		ingf = ImportedNGF::load(path);
		hngf = HomogenizedNGF::from(ingf);

		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal);

		if (program["--file"] == true) {
			bundle.file("examples/ngf/ngf.task", vk::ShaderStageFlagBits::eTaskEXT);
			bundle.file("examples/ngf/ngf.mesh", vk::ShaderStageFlagBits::eMeshEXT);
		} else {
			auto task_spv = link(task).generate(Target::spirv_binary_via_glsl, Stage::task);
			auto mesh_spv = link(mesh).generate(Target::spirv_binary_via_glsl, Stage::mesh);

			bundle.code(task_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eTaskEXT);
			bundle.code(mesh_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eMeshEXT);
			shader_debug();
		}

		auto fragment_spv = link(fragment).generate(Target::spirv_binary_via_glsl, Stage::fragment);

		bundle.code(fragment_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

		traditional = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
			(resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(bundle)
			.cull_mode(vk::CullModeFlagBits::eNone)
			.with_dsl_bindings(meshlet_bindings)
			.with_push_constant <solid_t <ViewInfo>> (vk::ShaderStageFlagBits::eMeshEXT
								  | vk::ShaderStageFlagBits::eTaskEXT);

		// Allocate textures for the NGF
		uint32_t patches = hngf.patches.size();
		uint32_t features = hngf.features.size() / patches;
		uint32_t vertices = hngf.vertices.size();
		uint32_t biases = hngf.biases.size();
		uint32_t wsize = hngf.W0.size() >> 6;

		JVL_INFO("neural geometry field with\n"
			"\tpatches={},\n"
			"\tvertices={},\n"
			"\tfeatures={},\n"
			"\tbiases={},\n"
			"\tweights={}",
			patches, vertices, features, biases, wsize);

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
		rtx_descriptor = littlevk::bind(resources.device, resources.descriptor_pool)
			.allocate_descriptor_sets(traditional.dsl.value()).front();

		vk::Sampler integer_sampler = littlevk::SamplerAssembler(resources.device, resources.dal)
			.filtering(vk::Filter::eNearest)
			.mipping(vk::SamplerMipmapMode::eNearest);

		vk::Sampler floating_sampler = littlevk::SamplerAssembler(resources.device, resources.dal);

		auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;

		littlevk::bind(resources.device, rtx_descriptor, meshlet_bindings)
			.queue_update(0, 0, integer_sampler, complex_texture.view, layout)
			.queue_update(1, 0, floating_sampler, vertex_texture.view, layout)
			.queue_update(2, 0, floating_sampler, feature_texture.view, layout)
			.queue_update(3, 0, floating_sampler, bias_texture.view, layout)
			.queue_update(4, 0, floating_sampler, W0_texture.view, layout)
			.queue_update(5, 0, floating_sampler, W1_texture.view, layout)
			.queue_update(6, 0, floating_sampler, W2_texture.view, layout)
			.queue_update(7, 0, floating_sampler, W3_texture.view, layout)
			.finalize();

		// Configure scene bounds
		min = glm::vec3(1e10);
		max = -min;

		for (auto &p4 : hngf.vertices) {
			auto p = 10.0f * glm::vec3(p4);
			min = glm::min(min, p);
			max = glm::max(max, p);
		}

		automatic = (program["auto"] == true);
	}

	void render(const vk::CommandBuffer &cmd, uint32_t index, uint32_t total) override {
		if (automatic) {
			auto &xform = camera.transform;

			float time = 0.01 * total;
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
		auto &extent = resources.window.extent;

		camera.aperature.aspect = float(extent.width)/float(extent.height);

		solid_t <ViewInfo> view_info;

		view_info.get <0> () = model_transform.matrix();
		view_info.get <1> () = camera.transform.view_matrix();
		view_info.get <2> () = camera.aperature.perspective();
		view_info.get <3> () = resolution;

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, traditional.handle);

		cmd.pushConstants <solid_t <ViewInfo>> (traditional.layout,
			vk::ShaderStageFlagBits::eTaskEXT
			| vk::ShaderStageFlagBits::eMeshEXT,
			0, view_info);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, traditional.layout, 0, rtx_descriptor, { });
		cmd.drawMeshTasksEXT(ingf.patch_count, 1, 1);

		imgui(cmd);

		cmd.endRenderPass();
	}

	void imgui(const vk::CommandBuffer &cmd) {
		ImGuiRenderContext context(cmd);

		ImGui::Begin("Neural Geometry Fields: Options");
		{
			ImGui::SliderInt("Resolution", &resolution, 2, 7);
			// TODO: tweak periodically if automatic
		}
		ImGui::End();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()
