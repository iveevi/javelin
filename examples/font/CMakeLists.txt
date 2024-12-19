add_executable(font main.cpp)
target_link_libraries(font javelin-ire common-gfx freetype)
target_compile_definitions(font PRIVATE $<$<CONFIG:DEBUG>:JVL_DEBUG=1>)