add_executable(particles main.cpp)

target_link_libraries(particles
	javelin-ire
	javelin-gfx
	OpenMP::OpenMP_CXX)