cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(javelin C CXX)

include(FetchContent)

FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG origin/main)

FetchContent_MakeAvailable(googletest)

add_executable(javelin
	source/exec/main.cpp
	source/core/preset.cpp)

add_executable(ire
	source/exec/ire.cpp
	source/ire/dump.cpp
	source/ire/reindex.cpp
	source/ire/translate_glsl.cpp
	thirdparty/glad/src/gl.c)

add_executable(test
	source/test/matrix.cpp)

include_directories(.
	include
	thirdparty/tinyobjloader
	thirdparty/glad/include)

target_link_libraries(javelin fmt)
target_link_libraries(ire fmt glfw)
target_link_libraries(test GTest::gtest_main)
