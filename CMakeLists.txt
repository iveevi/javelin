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

add_library(imgui SHARED
	thirdparty/imgui/imgui.cpp
	thirdparty/imgui/imgui_draw.cpp
	thirdparty/imgui/imgui_widgets.cpp
	thirdparty/imgui/imgui_demo.cpp
	thirdparty/imgui/imgui_tables.cpp
	thirdparty/imgui/backends/imgui_impl_glfw.cpp
	thirdparty/imgui/backends/imgui_impl_vulkan.cpp)

add_library(javelin-ire STATIC
	source/thunder/autodiff_forward.cpp
	source/thunder/comparison.cpp
	source/thunder/dump.cpp
	source/thunder/kernel.cpp
	source/thunder/linkage.cpp
	source/thunder/optimization.cpp
	source/thunder/reindex.cpp
	source/thunder/synthesize_glsl.cpp
	source/thunder/type_name.cpp
	source/thunder/usage.cpp
	source/ire/callable.cpp
	source/ire/compaction.cpp
	source/ire/emitter.cpp
	source/ire/marking.cpp
	source/ire/scratch.cpp)

add_executable(javelin
	source/core/aabb.cpp
	source/core/scene.cpp
	source/engine/imported_asset.cpp
	source/exec/javelin/attachment_inspectors.cpp
	source/exec/javelin/attachment_raytracing_cpu.cpp
	source/exec/javelin/attachment_ui.cpp
	source/exec/javelin/attachment_viewport.cpp
	source/exec/javelin/device_resource_collection.cpp
	source/exec/javelin/glio.cpp
	source/exec/javelin/main.cpp
	source/gfx/cpu/bvh.cpp
	source/gfx/cpu/intersection.cpp
	source/gfx/cpu/scene.cpp)

add_executable(ire-features
	source/exec/features/ire.cpp
	thirdparty/glad/src/gl.c)

add_executable(lang-features
	source/exec/features/lang.cpp
	source/lang/lex.cpp)

add_executable(vm-features
	source/exec/features/vm.cpp)

add_executable(test
	source/test/ire_emitter.cpp
	source/test/ire_synthesize_cpp.cpp
	source/test/ire_synthesize_glsl.cpp
	source/test/ire_ggx_brdf.cpp
	source/test/math_matrix.cpp
	thirdparty/glad/src/gl.c)

target_compile_options(lang-features PRIVATE
	-Wall
	-fno-rtti
	-fsanitize=address)

include_directories(.
	include
	thirdparty
	thirdparty/imgui
	thirdparty/tinyobjloader
	thirdparty/glad/include)

set(VULKAN_LIBRARY_SET
	glfw
	Vulkan::Vulkan
	SPIRV
	glslang
	glslang-default-resource-limits)

target_link_libraries(javelin-ire fmt::fmt)
target_link_libraries(javelin fmt::fmt ${VULKAN_LIBRARY_SET} imgui javelin-ire)
target_link_libraries(ire-features glfw fmt::fmt javelin-ire)
target_link_libraries(lang-features fmt::fmt -fsanitize=address)
target_link_libraries(vm-features fmt::fmt javelin-ire)
target_link_libraries(test GTest::gtest_main javelin-ire glfw)
