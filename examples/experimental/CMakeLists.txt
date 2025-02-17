set(ORIGIN ../..)

# Experimental program for the IRE system
add_executable(ire ire.cpp ${ORIGIN}/thirdparty/glad/src/gl.c)

set_property(TARGET ire PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(ire glfw fmt::fmt javelin-ire common-gfx gccjit)

# Experimental program for the TSG system
add_executable(tsg tsg.cpp ${ORIGIN}/thirdparty/glad/src/gl.c)

set_property(TARGET tsg PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(tsg glfw fmt::fmt javelin-ire common-gfx gccjit ${VULKAN_LIBRARY_SET})

target_compile_definitions(tsg PRIVATE $<$<CONFIG:DEBUG>:JVL_DEBUG=1>)

# Experimental program for higher order functions
add_executable(higher higher.cpp ${ORIGIN}/thirdparty/glad/src/gl.c)

set_property(TARGET higher PROPERTY ENABLE_EXPORTS ON)

target_link_libraries(higher glfw fmt::fmt javelin-ire common-gfx gccjit ${VULKAN_LIBRARY_SET})

target_compile_definitions(higher PRIVATE $<$<CONFIG:DEBUG>:JVL_DEBUG=1>)