add_executable(font main.cpp shaders.cpp)

target_link_libraries(font javelin common freetype)

target_compile_definitions(font PRIVATE $<$<CONFIG:DEBUG>:JVL_DEBUG=1>)
