add_executable(compute main.cpp shaders.cpp)

target_link_libraries(compute javelin common OpenMP::OpenMP_CXX)