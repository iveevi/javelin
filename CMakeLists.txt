set(CMAKE_CXX_STANDARD 20)

project(javelin C CXX)

include_directories(.
	include
	thirdparty/tinyobjloader
	thirdparty/glad/include)

add_executable(javelin source/exec/main.cpp)
add_executable(ire source/exec/ire.cpp thirdparty/glad/src/gl.c)

target_link_libraries(javelin fmt)
target_link_libraries(ire fmt glfw)
