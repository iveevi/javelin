cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(javelin C CXX)

add_compile_options(-fconcepts-diagnostics-depth=3)

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

add_library(jvl-ire STATIC
	source/atom/dump.cpp
	source/atom/kernel.cpp
	source/atom/reindex.cpp
	source/atom/synthesize_glsl.cpp
	source/ire/compaction.cpp
	source/ire/emitter.cpp)

add_executable(javelin
	source/exec/main.cpp
	source/core/preset.cpp
	source/gfx/cpu/bvh.cpp)

add_executable(ire-features
	source/exec/ire.cpp
	thirdparty/glad/src/gl.c)

add_executable(lang-features
	source/exec/lang.cpp
	source/lang/lex.cpp)

add_executable(test
	source/test/ire_synthesis.cpp
	source/test/math_matrix.cpp)

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

target_link_libraries(jvl-ire fmt::fmt)
target_link_libraries(javelin fmt::fmt ${VULKAN_LIBRARY_SET} imgui)
target_link_libraries(ire-features glfw fmt::fmt jvl-ire)
target_link_libraries(lang-features fmt::fmt -fsanitize=address)
target_link_libraries(test GTest::gtest_main jvl-ire)
