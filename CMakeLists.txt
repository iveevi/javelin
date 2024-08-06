cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(javelin C CXX)

add_compile_options(-fconcepts-diagnostics-depth=3)

find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)
find_package(glfw3 REQUIRED)

include(FetchContent)

FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG origin/main)

FetchContent_MakeAvailable(googletest)

add_library(imgui SHARED
	thirdparty/imgui/imgui.cpp
	thirdparty/imgui/imgui_draw.cpp
	thirdparty/imgui/imgui_widgets.cpp
	thirdparty/imgui/imgui_demo.cpp
	thirdparty/imgui/imgui_tables.cpp
	thirdparty/imgui/backends/imgui_impl_glfw.cpp
	thirdparty/imgui/backends/imgui_impl_vulkan.cpp)

add_executable(javelin
	source/exec/main.cpp
	source/core/preset.cpp)

add_executable(ire
	source/exec/ire.cpp
	source/ire/compaction.cpp
	source/ire/dump.cpp
	source/ire/emitter.cpp
	source/ire/reindex.cpp
	source/ire/translate_glsl.cpp
	thirdparty/glad/src/gl.c)

add_executable(test
	source/test/matrix.cpp)

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
	glslang::glslang
	glslang::glslang-default-resource-limits)

target_link_libraries(javelin fmt ${VULKAN_LIBRARY_SET} imgui)
target_link_libraries(ire fmt glfw)
target_link_libraries(test GTest::gtest_main)
