cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(javelin C CXX)

find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)

# Dependencies
if(NOT DEFINED JAVELIN_FMT_EXTERNAL)
	add_subdirectory(thirdparty/fmt)
endif()

if(NOT DEFINED JAVELIN_FREETYPE_EXTERNAL)
	add_subdirectory(thirdparty/freetype)
endif()

add_subdirectory(thirdparty/googletest EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/glslang EXCLUDE_FROM_ALL)

# Include directories
include_directories(PRIVATE .
	include
	thirdparty
	thirdparty/argparse/include
	thirdparty/freetype/include
	thirdparty/glad/include
	thirdparty/glm/
	thirdparty/imgui
	thirdparty/tinyobjloader)

# Bundle Vulkan necessities
set(VULKAN_LIBRARY_SET
	glfw
	Vulkan::Vulkan
	SPIRV
	glslang
	glslang-default-resource-limits)

# Options for coverage
set(STATIC_ANALYSIS)

if(ENABLE_COVERAGE)
	set(COVERAGE_FLAGS --coverage)
endif()

# Intermediate Representation Emitter
add_library(javelin STATIC
	source/common/debug.cpp
	source/common/io.cpp
	source/common/logging.cpp
	source/ire/emitter.cpp
	source/ire/native.cpp
	source/thunder/atom.cpp
	source/thunder/autodiff_forward.cpp
	source/thunder/buffer.cpp
	source/thunder/c_like_generator.cpp
	source/thunder/enumerations.cpp
	source/thunder/expansion.cpp
	source/thunder/gcc.cpp
	source/thunder/graphviz.cpp
	source/thunder/legalization.cpp
	source/thunder/legalize_cc.cpp
	source/thunder/linkage/common.cpp
	source/thunder/linkage/core.cpp
	source/thunder/linkage/cplusplus.cpp
	source/thunder/linkage/glsl.cpp
	source/thunder/linkage/jit_gcc.cpp
	source/thunder/linkage/spirv_via_glsl.cpp
	source/thunder/mark.cpp
	source/thunder/optimization.cpp
	source/thunder/overload_intrinsics.cpp
	source/thunder/overload_operations.cpp
	source/thunder/qualified_type.cpp
	source/thunder/semalz.cpp
	source/thunder/stitch.cpp
	source/thunder/tracked_buffer.cpp
	source/thunder/usage.cpp)

target_compile_options(javelin PRIVATE
	${STATIC_ANALYSIS}
	$<$<CONFIG:Debug>:-Wall;-Werror;${COVERAGE_FLAGS}>)

target_link_libraries(javelin
	fmt::fmt
	gccjit
	glslang::glslang
	glslang::glslang-default-resource-limits
	$<$<CONFIG:Debug>:${COVERAGE_FLAGS}>)

target_compile_definitions(javelin PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)

# Testing suite
add_subdirectory(testing EXCLUDE_FROM_ALL)

# Example projects
add_subdirectory(examples EXCLUDE_FROM_ALL)
