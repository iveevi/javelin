# Testing suite using GoogleTest
add_executable(test
	callable.cpp
	compute.cpp
	emitter.cpp
	ggx.cpp
	gl.cpp
	layouts_cpp.cpp
	layouts_glsl.cpp
	material_gcc.cpp
	solidification.cpp
	../thirdparty/glad/src/gl.c)

set_property(TARGET test PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(test
	GTest::gtest_main
	javelin-ire
	glfw
	gccjit
	$<$<CONFIG:Debug>:-fsanitize=address>)

target_compile_options(test PRIVATE
	$<$<CONFIG:Debug>:-Wall;-Werror;-fsanitize=address>)