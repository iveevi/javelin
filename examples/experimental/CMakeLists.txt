set(ORIGIN ../..)

add_executable(features features.cpp)
set_property(TARGET features PROPERTY ENABLE_EXPORTS ON)
target_link_libraries(features glfw fmt::fmt javelin common gccjit)
target_compile_definitions(features PRIVATE $<$<CONFIG:DEBUG>:JVL_DEBUG=1>)