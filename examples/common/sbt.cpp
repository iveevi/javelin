#include "sbt.hpp"

template <class I>
constexpr I align_up(I x, size_t a)
{
	return I((x + (I(a) - 1)) & ~I(a - 1));
}

std::pair <littlevk::Pipeline, ShaderBindingTable>
raytracing_pipeline(VulkanResources &resources,
		    const vk::ShaderModule &rgen,
		    const std::vector <vk::ShaderModule> &misses,
		    const std::vector <vk::ShaderModule> &hits,
		    const std::vector <vk::DescriptorSetLayoutBinding> &bindings,
		    const std::vector <vk::PushConstantRange> &ranges)
{
	ShaderBindingTable sbt;

	// Populate shaders stages and groups
	std::vector <vk::PipelineShaderStageCreateInfo> stages {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eRaygenKHR)
			.setPName("main")
			.setModule(rgen)
	};

	std::vector <vk::RayTracingShaderGroupCreateInfoKHR> groups {
		vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(0),
	};

	// Ray miss
	for (auto &m : misses) {
		auto stage = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eMissKHR)
			.setModule(m)
			.setPName("main");

		auto group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(stages.size());

		stages.emplace_back(stage);
		groups.emplace_back(group);
	}

	// Ray closest hit
	for (auto &m : hits) {
		auto stage = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
			.setModule(m)
			.setPName("main");

		auto group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
			.setClosestHitShader(stages.size());

		stages.emplace_back(stage);
		groups.emplace_back(group);
	}

	// Configure the pipeline
	littlevk::Pipeline pipeline;

	{
		auto info = vk::DescriptorSetLayoutCreateInfo().setBindings(bindings);
		pipeline.dsl = littlevk::descriptor_set_layout(resources.device, info).unwrap(resources.dal);
	}

	{
		auto info = vk::PipelineLayoutCreateInfo()
			.setSetLayouts(pipeline.dsl.value())
			.setPushConstantRanges(ranges);

		pipeline.layout = littlevk::pipeline_layout(resources.device, info).unwrap(resources.dal);
	}

	{
		auto info = vk::RayTracingPipelineCreateInfoKHR()
			.setStages(stages)
			.setGroups(groups)
			.setMaxPipelineRayRecursionDepth(1)
			.setLayout(pipeline.layout);

		pipeline.handle = resources.device.createRayTracingPipelinesKHR(nullptr, {}, info).value.front();
		littlevk::pipeline::PipelineReturnProxy(pipeline.handle).unwrap(resources.dal);
	}

	// Get handle sizes and alignment information
	auto pr_raytracing = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR();
	auto properties = vk::PhysicalDeviceProperties2KHR();
	properties.pNext = &pr_raytracing;

	resources.phdev.getProperties2(&properties);

	uint32_t handle_size = pr_raytracing.shaderGroupHandleSize;
	uint32_t handle_alignment = pr_raytracing.shaderGroupHandleAlignment;
	uint32_t base_alignment = pr_raytracing.shaderGroupBaseAlignment;
	uint32_t handle_size_aligned = align_up(handle_size, handle_alignment);

	// Prepare SBT records
	uint32_t ray_generation_size = align_up(handle_size_aligned, base_alignment);
	sbt.ray_generation = vk::StridedDeviceAddressRegionKHR()
		.setSize(ray_generation_size)
		.setStride(ray_generation_size);

	sbt.misses = vk::StridedDeviceAddressRegionKHR()
		.setSize(align_up(2 * handle_size_aligned, base_alignment))
		.setStride(handle_size_aligned);

	sbt.closest_hits = vk::StridedDeviceAddressRegionKHR()
		.setSize(align_up(handle_size_aligned, base_alignment))
		.setStride(handle_size_aligned);

	sbt.callables = vk::StridedDeviceAddressRegionKHR();

	// Get handle data
	std::vector <uint8_t> handles(handle_size * groups.size());

	auto _ = resources.device.getRayTracingShaderGroupHandlesKHR(pipeline.handle,
		0, groups.size(),
		handles.size(),
		handles.data());

	// Prepare SBT data
	uint32_t sbt_size = sbt.ray_generation.size
		+ sbt.misses.size
		+ sbt.closest_hits.size;

	std::vector <uint8_t> sbt_data(sbt_size);

	// Ray generation
	std::memcpy(sbt_data.data(), handles.data(), handle_size);

	// Miss
	size_t idx = 1;

	for (size_t i = 0; i < misses.size(); i++) {
		std::memcpy(sbt_data.data()
				+ sbt.ray_generation.size
				+ i * sbt.misses.stride,
			handles.data() + (idx++) * handle_size,
			handle_size);
	}

	// Closest hit
	for (size_t i = 0; i < hits.size(); i++) {
		std::memcpy(sbt_data.data()
				+ sbt.ray_generation.size
				+ sbt.misses.size
				+ i * sbt.closest_hits.stride,
			handles.data() + (idx++) * handle_size,
			handle_size);
	}

	// Upload to the GPU
	littlevk::Buffer sbt_buffer = resources.allocator()
		.buffer(sbt_data,
			vk::BufferUsageFlagBits::eShaderBindingTableKHR
			| vk::BufferUsageFlagBits::eShaderDeviceAddress);

	auto sbt_address = resources.device.getBufferAddress(sbt_buffer.buffer);

	sbt.ray_generation.setDeviceAddress(sbt_address);
	sbt.misses.setDeviceAddress(sbt_address + sbt.ray_generation.size);
	sbt.closest_hits.setDeviceAddress(sbt_address + sbt.ray_generation.size + sbt.misses.size);

	return std::make_pair(pipeline, sbt);
}
