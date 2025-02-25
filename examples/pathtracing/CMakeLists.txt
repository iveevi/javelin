add_executable(pathtracing main.cpp shaders.cpp)

target_link_libraries(pathtracing javelin common)