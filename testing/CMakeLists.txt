# Testing suite using GoogleTest
add_executable(test
	ire_callable.cpp
	ire_emitter.cpp
	ire_ggx_brdf.cpp
	ire_jit_gcc_material.cpp
	ire_solidification.cpp
	ire_synthesize_cpp.cpp
	ire_synthesize_glsl.cpp
	math_matrix.cpp
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