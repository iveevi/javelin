add_executable(particles main.cpp shaders.cpp)

target_link_libraries(particles javelin common OpenMP::OpenMP_CXX)