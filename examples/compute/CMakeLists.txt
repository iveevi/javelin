add_executable(compute main.cpp shaders.cpp)

target_link_libraries(compute javelin-ire common-gfx OpenMP::OpenMP_CXX)