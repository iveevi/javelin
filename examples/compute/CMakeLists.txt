add_executable(compute main.cpp)

target_link_libraries(compute
	javelin-ire
	javelin-gfx
	OpenMP::OpenMP_CXX)