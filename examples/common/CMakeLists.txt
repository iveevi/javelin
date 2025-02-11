set(ORIGIN ../..)

# ImGui sources for GLFW + Vulkan
set(IMGUI_SOURCES
	${ORIGIN}/thirdparty/imgui/imgui.cpp
	${ORIGIN}/thirdparty/imgui/imgui_draw.cpp
	${ORIGIN}/thirdparty/imgui/imgui_widgets.cpp
	${ORIGIN}/thirdparty/imgui/imgui_demo.cpp
	${ORIGIN}/thirdparty/imgui/imgui_tables.cpp
	${ORIGIN}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
	${ORIGIN}/thirdparty/imgui/backends/imgui_impl_vulkan.cpp)

# Javelin graphics stack
add_library(common-gfx STATIC
	${IMGUI_SOURCES}
	aabb.cpp
	camera_controller.cpp
	formats.cpp
	imgui.cpp
	imported_asset.cpp
	interactive_window.cpp
	texture.cpp
	vulkan/material.cpp
	vulkan/texture_bank.cpp
	vulkan_resources.cpp)

target_compile_options(common-gfx PRIVATE
	${STATIC_ANALYSIS}
	$<$<CONFIG:Debug>:-Wall;-Werror;${COVERAGE_FLAGS}>)

target_link_libraries(common-gfx PRIVATE
	fmt::fmt
	${VULKAN_LIBRARY_SET}
	$<$<CONFIG:Debug>:${COVERAGE_FLAGS}>)

target_compile_definitions(common-gfx PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)