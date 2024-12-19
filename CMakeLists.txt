cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(javelin C CXX)

find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)

include(FetchContent)

# Google testing library
FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG main)
FetchContent_MakeAvailable(googletest)

# Fmt printing library
FetchContent_Declare(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG master)
FetchContent_MakeAvailable(fmt)

# Native File Dialog (extended)
add_subdirectory(thirdparty/nativefiledialog-extended)

# FreeType2 for font loading
add_subdirectory(thirdparty/freetype)

add_subdirectory(thirdparty/glslang)

# Include directories
include_directories(.
	include
	thirdparty
	thirdparty/imgui
	thirdparty/tinyobjloader
	thirdparty/glad/include
	thirdparty/freetype/include
	thirdparty/nativefiledialog-extended/src/include)

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

# Intermediate Representation Emitter (IRE)
add_library(javelin-ire STATIC
	source/ire/emitter.cpp
	source/thunder/atom.cpp
	source/thunder/autodiff_forward.cpp
	source/thunder/buffer.cpp
	source/thunder/c_like_generator.cpp
	source/thunder/classify.cpp
	source/thunder/comparison.cpp
	source/thunder/gcc.cpp
	source/thunder/legalize_cc.cpp
	source/thunder/linkage_unit.cpp
	source/thunder/optimization.cpp
	source/thunder/overload_intrinsics.cpp
	source/thunder/overload_operations.cpp
	source/thunder/qualified_type.cpp
	source/thunder/stitch.cpp
	source/thunder/synthesized.cpp
	source/thunder/tracked_buffer.cpp
	source/thunder/usage.cpp)

target_compile_options(javelin-ire PRIVATE
	${STATIC_ANALYSIS}
	$<$<CONFIG:Debug>:-Wall;-Werror;${COVERAGE_FLAGS}>)

target_link_libraries(javelin-ire
	fmt::fmt
	gccjit
	glslang::glslang
	glslang::glslang-default-resource-limits
	$<$<CONFIG:Debug>:${COVERAGE_FLAGS}>)

target_compile_definitions(javelin-ire PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)

# Testing suite using GoogleTest
# TODO: move cmake file to source / tests
add_executable(test
	source/test/ire_callable.cpp
	source/test/ire_emitter.cpp
	source/test/ire_ggx_brdf.cpp
	source/test/ire_jit_gcc_material.cpp
	source/test/ire_solidification.cpp
	source/test/ire_synthesize_cpp.cpp
	source/test/ire_synthesize_glsl.cpp
	source/test/math_matrix.cpp
	thirdparty/glad/src/gl.c)

set_property(TARGET test PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(test
	GTest::gtest_main
	javelin-ire
	glfw
	gccjit
	$<$<CONFIG:Debug>:-fsanitize=address>)

target_compile_options(test PRIVATE
	$<$<CONFIG:Debug>:-Wall;-Werror;-fsanitize=address>)

# Example projects
add_subdirectory(examples)
