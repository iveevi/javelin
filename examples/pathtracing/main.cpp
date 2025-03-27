#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "common/acceleration_structure.hpp"
#include "common/aperature.hpp"
#include "common/application.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/sbt.hpp"
#include "common/vulkan_resources.hpp"
#include "common/vulkan_triangle_mesh.hpp"

#include "shaders.hpp"

struct Application : CameraApplication {
	littlevk::Pipeline rtx_pipeline;
	littlevk::Pipeline blit_pipeline;

	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;

	vk::DescriptorSet rtx_descriptor;
	vk::DescriptorSet blit_descriptor;

	littlevk::Image target;

	ShaderBindingTable sbt;
	VulkanAccelerationStructure tlas;

	uint32_t counter;
	glm::vec3 min;
	glm::vec3 max;
	bool automatic;

	static inline const std::vector <vk::DescriptorSetLayoutBinding> bindings {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
			.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR),
		vk::DescriptorSetLayoutBinding()
			.setBinding(1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR),
		vk::DescriptorSetLayoutBinding()
			.setBinding(2)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR),
	};

	Application() : CameraApplication("Pathtracing", {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		}, features_include, features_activate)
	{
		render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
			.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
			.add_attachment(littlevk::default_depth_attachment())
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.done();

		framebuffers.resize(resources, render_pass);

		configure_imgui(resources, render_pass);

		shader_debug();
		compile_rtx_pipeline();
		compile_blit_pipeline();
	}

	~Application() {
		tlas.destroy(resources.device);

		shutdown_imgui();
	}

	static void features_include(VulkanFeatureChain &features) {
		features.add <vk::PhysicalDeviceRayTracingPipelineFeaturesKHR> ();
		features.add <vk::PhysicalDeviceAccelerationStructureFeaturesKHR> ();
		features.add <vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR> ();
		features.add <vk::PhysicalDeviceScalarBlockLayoutFeaturesEXT> ();
	}

	static void features_activate(VulkanFeatureChain &features) {
		MODULE(features_activate);

		for (auto &ptr : features) {
			switch (ptr->sType) {
			feature_case(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR)
				.setRayTracingPipeline(true);
				break;
			feature_case(vk::PhysicalDeviceAccelerationStructureFeaturesKHR)
				.setAccelerationStructure(true);
				break;
			feature_case(vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR)
				.setBufferDeviceAddress(true)
				.setBufferDeviceAddressCaptureReplay(false)
				.setBufferDeviceAddressMultiDevice(false);
				break;
			feature_case(vk::PhysicalDeviceScalarBlockLayoutFeaturesEXT)
				.setScalarBlockLayout(true);
				break;
			default:
				break;
			}
		}
	}

	void compile_rtx_pipeline() {
		auto rgen_spv = link(ray_generation).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eRaygenKHR);
		auto rgen_info = vk::ShaderModuleCreateInfo().setCode(rgen_spv);
		auto rgen_module = resources.device.createShaderModule(rgen_info);
		littlevk::shader::ShaderModuleReturnProxy(rgen_module).unwrap(resources.dal);

		auto rchit_spv = link(primary_closest_hit).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eClosestHitKHR);
		auto rchit_info = vk::ShaderModuleCreateInfo().setCode(rchit_spv);
		auto rchit_module = resources.device.createShaderModule(rchit_info);
		littlevk::shader::ShaderModuleReturnProxy(rchit_module).unwrap(resources.dal);

		auto rmiss_spv = link(primary_miss).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eMissKHR);
		auto rmiss_info = vk::ShaderModuleCreateInfo().setCode(rmiss_spv);
		auto rmiss_module = resources.device.createShaderModule(rmiss_info);
		littlevk::shader::ShaderModuleReturnProxy(rmiss_module).unwrap(resources.dal);

		auto smiss_spv = link(shadow_miss).generate_spirv_via_glsl(vk::ShaderStageFlagBits::eMissKHR);
		auto smiss_info = vk::ShaderModuleCreateInfo().setCode(smiss_spv);
		auto smiss_module = resources.device.createShaderModule(smiss_info);
		littlevk::shader::ShaderModuleReturnProxy(smiss_module).unwrap(resources.dal);

		auto constants = vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(solid_t <Constants>))
			.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

		std::tie(rtx_pipeline, sbt) = raytracing_pipeline(resources,
			rgen_module,
			{ rmiss_module, smiss_module },
			{ rchit_module },
			bindings,
			{ constants });

		rtx_descriptor = littlevk::bind(resources.device, resources.descriptor_pool)
			.allocate_descriptor_sets(*rtx_pipeline.dsl)
			.front();
	}

	void compile_blit_pipeline() {
		std::string quad_shader = link(quad).generate_glsl();
		std::string blit_shader = link(blit).generate_glsl();

		auto blit_bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(quad_shader, vk::ShaderStageFlagBits::eVertex)
			.source(blit_shader, vk::ShaderStageFlagBits::eFragment);

		blit_pipeline = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
			(resources.device, resources.window, resources.dal)
			.with_shader_bundle(blit_bundle)
			.with_render_pass(render_pass, 0)
			.with_dsl_binding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

		blit_descriptor = littlevk::bind(resources.device, resources.descriptor_pool)
			.allocate_descriptor_sets(*blit_pipeline.dsl)
			.front();
	}

	void configure(argparse::ArgumentParser &program) override {
		program.add_argument("mesh")
			.help("input mesh");

		program.add_argument("--smooth")
			.help("smooth mesh normals")
			.default_value(false)
			.flag();
	}

	void preload(const argparse::ArgumentParser &program) override {
		// Load the asset and scene
		std::filesystem::path path = program.get("mesh");

		auto asset = ImportedAsset::from(path).value();

		std::vector <VulkanTriangleMesh> vk_meshes;

		struct Ref {
			uint64_t vertices;
			uint64_t triangles;
		};

		std::vector <Ref> references;

		min = glm::vec3(1e10);
		max = -min;

		bool smooth = (program["--smooth"] == true);

		for (auto &g : asset.geometries) {
			if (smooth)
				g.deduplicate_vertices().recompute_normals();

			auto tm = TriangleMesh::from(g).value();

			auto vkm = VulkanTriangleMesh::from(resources.allocator(), tm,
				VertexFlags::ePosition
				| VertexFlags::eNormal,
				vk::BufferUsageFlagBits::eShaderDeviceAddress
				| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR).value();

			vk_meshes.emplace_back(vkm);

			auto taddr = resources.device.getBufferAddress(vkm.triangles.buffer);
			auto vaddr = resources.device.getBufferAddress(vkm.vertices.buffer);
			references.emplace_back(vaddr, taddr);

			auto [bmin, bmax] = tm.scale();
			min = glm::min(min, bmin);
			max = glm::max(max, bmax);
		}

		std::vector <VulkanAccelerationStructure> blases;
		std::vector <vk::TransformMatrixKHR> transforms;
		std::vector <InstanceInfo> instances;

		littlevk::submit_now(resources.device, resources.command_pool, resources.graphics_queue,
			[&](const vk::CommandBuffer &cmd) {
				for (size_t i = 0; i < vk_meshes.size(); i++) {
					auto model = Transform().matrix();
					auto blas = VulkanAccelerationStructure::blas(resources, cmd, vk_meshes[i], 2 * sizeof(glm::vec3));
					blases.emplace_back(blas);
					transforms.emplace_back(vk_transform(model));
					instances.emplace_back(i, 0xff);
				}
			});

		littlevk::submit_now(resources.device, resources.command_pool, resources.graphics_queue,
			[&](const vk::CommandBuffer &cmd) {
				tlas = VulkanAccelerationStructure::tlas(resources, cmd, blases, transforms, instances);
			});

		for (auto &blas : blases)
			blas.destroy(resources.device);

		littlevk::ImageCreateInfo image_info {
			resources.window.extent.width,
			resources.window.extent.height,
			vk::Format::eR32G32B32A32Sfloat,
			vk::ImageUsageFlagBits::eStorage
			| vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eColor,
			vk::ImageType::e2D,
			vk::ImageViewType::e2D,
			false
		};

		target = resources.allocator().image(image_info);

		littlevk::Buffer vk_refs = resources.allocator()
			.buffer(references, vk::BufferUsageFlagBits::eStorageBuffer);

		littlevk::submit_now(resources.device, resources.command_pool, resources.graphics_queue,
			[&](const vk::CommandBuffer &cmd) {
				target.transition(cmd, vk::ImageLayout::eGeneral);
			});

		auto tlas_write = vk::WriteDescriptorSetAccelerationStructureKHR()
			.setAccelerationStructureCount(1)
			.setAccelerationStructures(tlas.handle);

		auto target_sampler = littlevk::SamplerAssembler(resources.device, resources.dal);

		std::vector <vk::WriteDescriptorSet> writes;

		writes.emplace_back(vk::WriteDescriptorSet()
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
			.setDstBinding(0)
			.setDstSet(rtx_descriptor)
			.setPNext(&tlas_write));

		auto target_descriptor1 = vk::DescriptorImageInfo()
			.setImageView(target.view)
			.setImageLayout(vk::ImageLayout::eGeneral)
			.setSampler(target_sampler);

		writes.emplace_back(vk::WriteDescriptorSet()
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setDstBinding(1)
			.setDstSet(rtx_descriptor)
			.setImageInfo(target_descriptor1));

		auto target_descriptor2 = vk::DescriptorImageInfo()
			.setImageView(target.view)
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSampler(target_sampler);

		writes.emplace_back(vk::WriteDescriptorSet()
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDstBinding(0)
			.setDstSet(blit_descriptor)
			.setImageInfo(target_descriptor2));

		auto refs_descriptor = vk::DescriptorBufferInfo()
			.setBuffer(vk_refs.buffer)
			.setOffset(0)
			.setRange(vk_refs.device_size());

		writes.emplace_back(vk::WriteDescriptorSet()
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDstBinding(2)
			.setDstSet(rtx_descriptor)
			.setBufferInfo(refs_descriptor));

		resources.device.updateDescriptorSets(writes, { });

		automatic = (program["--auto"] == true);

		counter = 0;
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

			counter = 0;
		} else {
			if (camera.controller.handle_movement(resources.window))
				counter = 0;
		}

		if (camera.controller.dragging)
			counter = 0;

		auto subresource_range = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setBaseMipLevel(0)
			.setLayerCount(1)
			.setLevelCount(1);

		solid_t <Constants> frame;

		auto &extent = resources.window.extent;
		camera.aperature.aspect = float(extent.width) / float(extent.height);

		auto rayframe = camera.aperature.rayframe(camera.transform);

		frame.get <0> () = rayframe.origin;
		frame.get <1> () = rayframe.lower_left;
		frame.get <2> () = rayframe.horizontal;
		frame.get <3> () = rayframe.vertical;
		frame.get <5> () = glfwGetTime();
		frame.get <6> () = counter++;

		{
			// float time = glfwGetTime();
			float time = 0.0f;

			glm::vec3 direction = glm::vec3 {
				0.1 * glm::cos(time),
				-1,
				0.1 * glm::sin(time),
			};

			frame.get <4> () = glm::normalize(direction);
		}

		{
			auto memory_barrier = vk::ImageMemoryBarrier()
				.setImage(target.image)
				.setSubresourceRange(subresource_range)
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eGeneral)
				.setSrcAccessMask(vk::AccessFlagBits::eNone)
				.setDstAccessMask(vk::AccessFlagBits::eShaderWrite);

			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eRayTracingShaderKHR,
				vk::DependencyFlags(),
				{ }, { }, memory_barrier);
		}

		cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtx_pipeline.handle);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtx_pipeline.layout, 0, rtx_descriptor, { });
		cmd.pushConstants <solid_t <Constants>> (rtx_pipeline.layout, vk::ShaderStageFlagBits::eRaygenKHR, 0, frame);

		cmd.traceRaysKHR(sbt.ray_generation,
			sbt.misses,
			sbt.closest_hits,
			sbt.callables,
			target.extent.width, target.extent.height, 1);

		{
			auto memory_barrier = vk::ImageMemoryBarrier()
				.setImage(target.image)
				.setSubresourceRange(subresource_range)
				.setOldLayout(vk::ImageLayout::eGeneral)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eRayTracingShaderKHR,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags(),
				{ }, { }, memory_barrier);
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

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, blit_pipeline.handle);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blit_pipeline.layout, 0, blit_descriptor, { });
		cmd.draw(6, 1, 0, 0);

		imgui(cmd);

		cmd.endRenderPass();
	}

	void imgui(const vk::CommandBuffer &cmd) {
		ImGuiRenderContext context(cmd);

		ImGui::Begin("Path Tracing: Options");
		{
			// TODO: rendering modes... (AO, plus tonemapping configurations)
		}
		ImGui::End();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()
