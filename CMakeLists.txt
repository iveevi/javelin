cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(javelin C CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

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

# Include directories
include_directories(.
	include
	thirdparty
	thirdparty/imgui
	thirdparty/tinyobjloader
	thirdparty/glad/include
	thirdparty/nativefiledialog-extended/src/include)

# Bundle Vulkan necessities
set(VULKAN_LIBRARY_SET
	glfw
	Vulkan::Vulkan
	SPIRV
	glslang
	glslang-default-resource-limits)

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
	-fno-rtti
	$<$<CONFIG:Debug>:-Wall;-Werror;--coverage>)

target_link_libraries(javelin-ire
	fmt::fmt
	gccjit
	$<$<CONFIG:Debug>:--coverage>)

target_compile_definitions(javelin-ire PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)

# ImGui sources for GLFW + Vulkan
set(IMGUI_SOURCES
	thirdparty/imgui/imgui.cpp
	thirdparty/imgui/imgui_draw.cpp
	thirdparty/imgui/imgui_widgets.cpp
	thirdparty/imgui/imgui_demo.cpp
	thirdparty/imgui/imgui_tables.cpp
	thirdparty/imgui/backends/imgui_impl_glfw.cpp
	thirdparty/imgui/backends/imgui_impl_vulkan.cpp)

# Javelin graphics stack
add_library(javelin-gfx STATIC
	${IMGUI_SOURCES}
	source/core/aabb.cpp
	source/core/scene.cpp
	source/core/texture.cpp
	source/engine/camera_controller.cpp
	source/engine/device_resource_collection.cpp
	source/engine/imgui.cpp
	source/engine/imported_asset.cpp
	source/engine/interactive_window.cpp
	source/gfx/cpu/bvh.cpp
	source/gfx/cpu/intersection.cpp
	source/gfx/cpu/scene.cpp)

target_compile_options(javelin-gfx PRIVATE
	-fno-rtti
	$<$<CONFIG:Debug>:-Wall;-Werror;--coverage>)

target_link_libraries(javelin-gfx PRIVATE
	fmt::fmt
	${VULKAN_LIBRARY_SET}
	$<$<CONFIG:Debug>:--coverage>)

target_compile_definitions(javelin-gfx PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)

# Javelin editor
add_executable(editor
	source/exec/editor/editor.cpp
	source/exec/editor/imgui_render_group.cpp
	source/exec/editor/main.cpp
	source/exec/editor/raytracer_cpu.cpp
	source/exec/editor/scene_inspector.cpp
	source/exec/editor/viewport.cpp
	source/exec/editor/viewport_render_group.cpp)

target_compile_options(editor PRIVATE
	-fno-rtti
	$<$<CONFIG:Debug>:-Wall;-Werror;--coverage>)

target_link_libraries(editor PRIVATE
	nfd
	fmt::fmt
	javelin-ire
	javelin-gfx
	$<$<CONFIG:Debug>:--coverage>)

target_compile_definitions(editor PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)

# Experimental program for the IRE system
add_executable(ire-features
	source/exec/features/ire.cpp
	thirdparty/glad/src/gl.c)

set_property(TARGET ire-features PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(ire-features glfw fmt::fmt javelin-ire gccjit)

# Experimental program for the JVL language
add_executable(lang-features
	source/exec/features/lang.cpp
	source/lang/lex.cpp)

target_compile_options(lang-features PRIVATE
	-fno-rtti
	$<$<CONFIG:Debug>:-Wall;-Werror;-fsanitize=address>)

target_link_libraries(lang-features PRIVATE
	fmt::fmt
	$<$<CONFIG:Debug>:-fsanitize=address>)

# Testing suite using GoogleTest
add_executable(test
	source/test/ire_callable.cpp
	source/test/ire_emitter.cpp
	source/test/ire_jit_gcc_material.cpp
	source/test/ire_synthesize_cpp.cpp
	source/test/ire_synthesize_glsl.cpp
	source/test/ire_ggx_brdf.cpp
	source/test/math_matrix.cpp
	thirdparty/glad/src/gl.c)

set_property(TARGET test PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(test
	GTest::gtest_main
	javelin-ire
	glfw
	gccjit)

# Example projects
add_subdirectory(examples)