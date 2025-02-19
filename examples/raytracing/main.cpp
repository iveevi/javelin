#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "common/aperature.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/vulkan_resources.hpp"
#include "common/application.hpp"
#include "common/util.hpp"
#include "common/vulkan_triangle_mesh.hpp"

#include "shaders.hpp"

vk::TransformMatrixKHR vk_transform(const glm::mat4 &m)
{
	return vk::TransformMatrixKHR()
		.setMatrix(std::array <std::array <float, 4>, 3> {
			std::array <float, 4> { m[0][0], m[1][0], m[2][0], m[3][0] },
			std::array <float, 4> { m[0][1], m[1][1], m[2][1], m[3][1] },
			std::array <float, 4> { m[0][2], m[1][2], m[2][2], m[3][2] }
		});
}

struct InstanceInfo {
	uint32_t index;
	uint32_t mask;
};

struct VulkanAccelerationStructure {
	vk::AccelerationStructureKHR handle;
	vk::Buffer buffer;
	vk::DeviceMemory memory;
	vk::DeviceSize size;

	static VulkanAccelerationStructure blas(VulkanResources &,
		const vk::CommandBuffer &,
		const VulkanTriangleMesh &);

	static VulkanAccelerationStructure tlas(VulkanResources &,
		const vk::CommandBuffer &,
		const std::vector <VulkanAccelerationStructure> &,
		const std::vector <vk::TransformMatrixKHR> &,
		const std::vector <InstanceInfo> &);
};

VulkanAccelerationStructure VulkanAccelerationStructure::blas(VulkanResources &resources,
							      const vk::CommandBuffer &cmd,
							      const VulkanTriangleMesh &tm)
{
	auto &device = resources.device;

	auto build_type = vk::AccelerationStructureBuildTypeKHR::eDevice;

	auto triangles_data = vk::AccelerationStructureGeometryTrianglesDataKHR()
		.setVertexData(device.getBufferAddress(tm.vertices.buffer))
		.setIndexData(device.getBufferAddress(tm.triangles.buffer))
		.setIndexType(vk::IndexType::eUint32)
		.setMaxVertex(tm.vertex_count)
		.setVertexStride(sizeof(glm::vec3))
		.setVertexFormat(vk::Format::eR32G32B32Sfloat);

	auto blas_geometry_data = vk::AccelerationStructureGeometryDataKHR()
		.setTriangles(triangles_data);

	auto blas_geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometry(blas_geometry_data)
		.setGeometryType(vk::GeometryTypeKHR::eTriangles);

	auto blas_build_info = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
		.setGeometries(blas_geometry);

	auto blas_build_sizes = device.getAccelerationStructureBuildSizesKHR(build_type, blas_build_info, tm.triangle_count);

	littlevk::Buffer blas_buffer = resources.allocator()
		.buffer(blas_build_sizes.accelerationStructureSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
			| vk::BufferUsageFlagBits::eShaderDeviceAddress);

	auto blas_info = vk::AccelerationStructureCreateInfoKHR()
		.setBuffer(blas_buffer.buffer)
		.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
		.setSize(blas_build_sizes.accelerationStructureSize);

	auto blas = device.createAccelerationStructureKHR(blas_info);
	
	littlevk::Buffer blas_scratch_buffer = resources.allocator()
		.buffer(blas_build_sizes.buildScratchSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
			| vk::BufferUsageFlagBits::eShaderDeviceAddress
			| vk::BufferUsageFlagBits::eStorageBuffer);

	blas_build_info = blas_build_info
		.setDstAccelerationStructure(blas)
		.setScratchData(device.getBufferAddress(blas_scratch_buffer.buffer));

	auto blas_build_range = vk::AccelerationStructureBuildRangeInfoKHR()
		.setFirstVertex(0)
		.setPrimitiveCount(tm.triangle_count)
		.setPrimitiveOffset(0)
		.setTransformOffset(0);

	cmd.buildAccelerationStructuresKHR(blas_build_info, &blas_build_range);

	return VulkanAccelerationStructure {
		.handle = blas,
		.buffer = blas_buffer.buffer,
		.memory = blas_buffer.memory,
		.size = blas_buffer.device_size(),
	};
}

VulkanAccelerationStructure VulkanAccelerationStructure::tlas(VulkanResources &resources,
							      const vk::CommandBuffer &cmd,
							      const std::vector <VulkanAccelerationStructure> &blases,
							      const std::vector <vk::TransformMatrixKHR> &transforms,
							      const std::vector <InstanceInfo> &infos)
{
	MODULE(tlas);

	JVL_ASSERT(blases.size() == transforms.size(), "mismatch between blases and transforms sizes");

	auto &device = resources.device;

	std::vector <vk::AccelerationStructureInstanceKHR> instances;
	for (size_t i = 0; i < blases.size(); i++) {
		auto instance = vk::AccelerationStructureInstanceKHR()
			.setInstanceShaderBindingTableRecordOffset(0)
			.setInstanceCustomIndex(infos[i].index)
			.setAccelerationStructureReference(device.getAccelerationStructureAddressKHR(blases[i].handle))
			.setMask(infos[i].mask)
			.setTransform(transforms[i])
			.setFlags(vk::GeometryInstanceFlagBitsKHR::eForceOpaque
				| vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);

		instances.push_back(instance);
	}

	JVL_ASSERT(instances.size() > 0, "zero instances provided to tlas constructor");
	
	littlevk::Buffer instance_buffer = resources.allocator()
		.buffer(instances,
			vk::BufferUsageFlagBits::eStorageBuffer
			| vk::BufferUsageFlagBits::eShaderDeviceAddress
			| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
	
	auto instance_data = vk::AccelerationStructureGeometryInstancesDataKHR()
		.setData(device.getBufferAddress(instance_buffer.buffer));
		
	auto tlas_geometry_data = vk::AccelerationStructureGeometryDataKHR()
		.setInstances(instance_data);

	auto tlas_geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometryType(vk::GeometryTypeKHR::eInstances)
		.setGeometry(tlas_geometry_data);

	auto tlas_build_info = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
		.setGeometries(tlas_geometry);
	
	auto tlas_build_sizes = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, tlas_build_info, instances.size());
	
	littlevk::Buffer tlas_buffer = resources.allocator()
		.buffer(tlas_build_sizes.accelerationStructureSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
			| vk::BufferUsageFlagBits::eShaderDeviceAddress);

	auto tlas_info = vk::AccelerationStructureCreateInfoKHR()
		.setBuffer(tlas_buffer.buffer)
		.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
		.setSize(tlas_build_sizes.accelerationStructureSize);

	auto tlas = device.createAccelerationStructureKHR(tlas_info);
	
	littlevk::Buffer tlas_scratch_buffer = resources.allocator()
		.buffer(tlas_build_sizes.buildScratchSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
			| vk::BufferUsageFlagBits::eShaderDeviceAddress
			| vk::BufferUsageFlagBits::eStorageBuffer);

	tlas_build_info = tlas_build_info
		.setDstAccelerationStructure(tlas)
		.setScratchData(device.getBufferAddress(tlas_scratch_buffer.buffer));

	auto tlas_build_range = vk::AccelerationStructureBuildRangeInfoKHR()
		.setFirstVertex(0)
		.setPrimitiveCount(instances.size())
		.setPrimitiveOffset(0)
		.setTransformOffset(0);

	cmd.buildAccelerationStructuresKHR(tlas_build_info, &tlas_build_range);

	return VulkanAccelerationStructure {
		.handle = tlas,
		.buffer = tlas_buffer.buffer,
		.memory = tlas_buffer.memory,
		.size = tlas_buffer.device_size(),
	};
}

struct ShaderBindingTable {
	vk::StridedDeviceAddressRegionKHR ray_generation;
	vk::StridedDeviceAddressRegionKHR misses;
	vk::StridedDeviceAddressRegionKHR closest_hits;
	vk::StridedDeviceAddressRegionKHR callables;
};

template <class I>
constexpr I align_up(I x, size_t a)
{
	return I((x + (I(a) - 1)) & ~I(a - 1));
}

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
	VulkanAccelerationStructure blas;

	VulkanTriangleMesh vtm;

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
	};

	Application() : CameraApplication("Raytracing", {
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
	
		compile_rtx_pipeline();
		compile_blit_pipeline();
	}

	static void features_include(VulkanFeatureChain &features) {
		features.add <vk::PhysicalDeviceRayTracingPipelineFeaturesKHR> ();
		features.add <vk::PhysicalDeviceAccelerationStructureFeaturesKHR> ();
		features.add <vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR> ();
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
			default:
				break;
			}
		}
	}

	void compile_rtx_pipeline() {
		std::string rgen_shader = link(ray_generation).generate_glsl();
		std::string rchit_shader = link(ray_closest_hit).generate_glsl();
		std::string rmiss_shader = link(ray_miss).generate_glsl();

		dump_lines("RAY GENERATION", rgen_shader);
		dump_lines("RAY CLOSEST HIT", rchit_shader);
		dump_lines("RAY MISS", rmiss_shader);
		
		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(rgen_shader, vk::ShaderStageFlagBits::eRaygenKHR)
			.source(rmiss_shader, vk::ShaderStageFlagBits::eMissKHR)
			.source(rchit_shader, vk::ShaderStageFlagBits::eClosestHitKHR);

		auto rgen_group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(0);
		
		auto rmiss_group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(1);
		
		auto rchit_group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
			.setClosestHitShader(2);

		std::vector <vk::RayTracingShaderGroupCreateInfoKHR> groups {
			rgen_group,
			rmiss_group,
			rchit_group,
		};

		auto pc_range = vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(solid_t <ViewFrame>))
			.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

		auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(bindings);

		rtx_pipeline.dsl = resources.device.createDescriptorSetLayout(dsl_info);

		auto layout_info = vk::PipelineLayoutCreateInfo()
			.setPushConstantRanges(pc_range)
			.setSetLayouts(*rtx_pipeline.dsl);

		rtx_pipeline.layout = resources.device.createPipelineLayout(layout_info);

		auto pipeline_info = vk::RayTracingPipelineCreateInfoKHR()
			.setStages(bundle.stages)
			.setGroups(groups)
			.setMaxPipelineRayRecursionDepth(1)
			.setLayout(rtx_pipeline.layout);

		rtx_pipeline.handle = resources.device.createRayTracingPipelineKHR(nullptr, { }, pipeline_info).value;

		auto pr_raytracing = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR();
		auto properties = vk::PhysicalDeviceProperties2KHR();

		properties.pNext = &pr_raytracing;

		resources.phdev.getProperties2(&properties);

		uint32_t handle_size = pr_raytracing.shaderGroupHandleSize;
		uint32_t handle_alignment = pr_raytracing.shaderGroupHandleAlignment;
		uint32_t base_alignment = pr_raytracing.shaderGroupBaseAlignment;
		uint32_t handle_size_aligned = align_up(handle_size, handle_alignment);

		uint32_t ray_generation_size = align_up(handle_size_aligned, base_alignment);
		sbt.ray_generation = vk::StridedDeviceAddressRegionKHR()
			.setSize(ray_generation_size)
			.setStride(ray_generation_size);

		sbt.misses = vk::StridedDeviceAddressRegionKHR()
			.setSize(align_up(handle_size_aligned, base_alignment))
			.setStride(handle_size_aligned);
		
		sbt.closest_hits = vk::StridedDeviceAddressRegionKHR()
			.setSize(align_up(handle_size_aligned, base_alignment))
			.setStride(handle_size_aligned);
		
		sbt.callables = vk::StridedDeviceAddressRegionKHR();

		std::vector <uint8_t> handles(handle_size * groups.size());

		auto result = resources.device.getRayTracingShaderGroupHandlesKHR(rtx_pipeline.handle,
			0, groups.size(),
			handles.size(),
			handles.data());

		uint32_t sbt_size = sbt.ray_generation.size
			+ sbt.misses.size
			+ sbt.closest_hits.size;

		std::vector <uint8_t> sbt_data(sbt_size);

		// Ray generation
		std::memcpy(sbt_data.data(), handles.data(), handle_size);

		// Miss
		std::memcpy(sbt_data.data() + sbt.ray_generation.size,
			handles.data() + handle_size,
			handle_size);

		// Closest hit
		std::memcpy(sbt_data.data()
				+ sbt.ray_generation.size
				+ sbt.misses.size,
			handles.data() + 2 * handle_size,
			handle_size);

		// Upload to the GPU
		littlevk::Buffer sbt_buffer = resources.allocator()
			.buffer(sbt_data,
				vk::BufferUsageFlagBits::eShaderBindingTableKHR
				| vk::BufferUsageFlagBits::eShaderDeviceAddress);

		auto sbt_address = resources.device.getBufferAddress(sbt_buffer.buffer);

		sbt.ray_generation.setDeviceAddress(sbt_address);
		sbt.misses.setDeviceAddress(sbt_address + sbt.ray_generation.size);
		sbt.closest_hits.setDeviceAddress(sbt_address + sbt.ray_generation.size + sbt.misses.size);
		
		rtx_descriptor = littlevk::bind(resources.device, resources.descriptor_pool)
			.allocate_descriptor_sets(*rtx_pipeline.dsl)
			.front();
	}

	void compile_blit_pipeline() {
		std::string quad_shader = link(quad).generate_glsl();
		std::string blit_shader = link(blit).generate_glsl();

		dump_lines("QUAD", quad_shader);
		dump_lines("BLIT", blit_shader);

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
	}
	
	void preload(const argparse::ArgumentParser &program) override {
		// Load the asset and scene
		std::filesystem::path path = program.get("mesh");
	
		auto asset = ImportedAsset::from(path).value();

		// TODO: entire models
		auto &g = asset.geometries[0];
		auto tm = TriangleMesh::from(g).value();
		
		vtm = VulkanTriangleMesh::from(resources.allocator(), tm,
			VertexFlags::ePosition,
			vk::BufferUsageFlagBits::eShaderDeviceAddress
			| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR).value();
		
		littlevk::submit_now(resources.device, resources.command_pool, resources.graphics_queue,
			[&](const vk::CommandBuffer &cmd) {
				blas = VulkanAccelerationStructure::blas(resources, cmd, vtm);
			});
		
		littlevk::submit_now(resources.device, resources.command_pool, resources.graphics_queue,
			[&](const vk::CommandBuffer &cmd) {
				std::vector <vk::TransformMatrixKHR> transforms;
				std::vector <InstanceInfo> instances;
				
				auto model = Transform().matrix();
				
				transforms.emplace_back(vk_transform(model));
				instances.emplace_back(0, 0xff);

				tlas = VulkanAccelerationStructure::tlas(resources, cmd, { blas }, transforms, instances);
			});

		littlevk::ImageCreateInfo image_info {
			1000,
			1000,
			vk::Format::eR32G32B32A32Sfloat,
			vk::ImageUsageFlagBits::eStorage
			| vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eColor,
			vk::ImageType::e2D,
			vk::ImageViewType::e2D,
			false
		};

		target = resources.allocator().image(image_info);

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

		resources.device.updateDescriptorSets(writes, { });
	}

	void render(const vk::CommandBuffer &cmd, uint32_t index) override {
		camera.controller.handle_movement(resources.window);

		auto subresource_range = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setBaseMipLevel(0)
			.setLayerCount(1)
			.setLevelCount(1);
		
		solid_t <ViewFrame> frame;
		
		auto &extent = resources.window.extent;
		camera.aperature.aspect = float(extent.width) / float(extent.height);

		auto rayframe = camera.aperature.rayframe(camera.transform);

		frame.get <0> () = rayframe.origin;
		frame.get <1> () = rayframe.lower_left;
		frame.get <2> () = rayframe.horizontal;
		frame.get <3> () = rayframe.vertical;
		
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
		cmd.pushConstants <solid_t <ViewFrame>> (rtx_pipeline.layout, vk::ShaderStageFlagBits::eRaygenKHR, 0, frame);

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

		ImGui::Begin("Ray Tracing: Options");
		{
			// TODO: rendering modes...
		}
		ImGui::End();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()