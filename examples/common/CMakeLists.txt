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
add_library(common STATIC
	${IMGUI_SOURCES}
	acceleration_structure.cpp
	camera_controller.cpp
	formats.cpp
	imgui.cpp
	imported_asset.cpp
	imported_mesh.cpp
	interactive_window.cpp
	material.cpp
	sbt.cpp
	texture.cpp
	texture_bank.cpp
	triangle_mesh.cpp
	vulkan_resources.cpp
	vulkan_triangle_mesh.cpp)

target_compile_options(common PRIVATE
	${STATIC_ANALYSIS}
	$<$<CONFIG:Debug>:-Wall;-Werror;${COVERAGE_FLAGS}>)

target_link_libraries(common PRIVATE
	fmt::fmt
	${VULKAN_LIBRARY_SET}
	$<$<CONFIG:Debug>:${COVERAGE_FLAGS}>)

target_compile_definitions(common PRIVATE
	$<$<CONFIG:DEBUG>:JVL_DEBUG=1>)