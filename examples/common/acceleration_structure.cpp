#include "acceleration_structure.hpp"

void VulkanAccelerationStructure::destroy(const vk::Device &device)
{
	device.destroyAccelerationStructureKHR(handle);
}

vk::TransformMatrixKHR vk_transform(const glm::mat4 &m)
{
	return vk::TransformMatrixKHR()
		.setMatrix(std::array <std::array <float, 4>, 3> {
			std::array <float, 4> { m[0][0], m[1][0], m[2][0], m[3][0] },
			std::array <float, 4> { m[0][1], m[1][1], m[2][1], m[3][1] },
			std::array <float, 4> { m[0][2], m[1][2], m[2][2], m[3][2] }
		});
}

VulkanAccelerationStructure VulkanAccelerationStructure::blas(VulkanResources &resources,
							      const vk::CommandBuffer &cmd,
							      const VulkanTriangleMesh &tm,
							      size_t vertex_stride)
{
	auto &device = resources.device;

	auto build_type = vk::AccelerationStructureBuildTypeKHR::eDevice;

	auto triangles_data = vk::AccelerationStructureGeometryTrianglesDataKHR()
		.setVertexData(device.getBufferAddress(tm.vertices.buffer))
		.setIndexData(device.getBufferAddress(tm.triangles.buffer))
		.setIndexType(vk::IndexType::eUint32)
		.setMaxVertex(tm.vertex_count)
		.setVertexStride(vertex_stride)
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
