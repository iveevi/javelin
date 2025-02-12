add_executable(particles main.cpp shaders.cpp)

target_link_libraries(particles javelin-ire common-gfx OpenMP::OpenMP_CXX)