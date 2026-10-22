# pfl_render: OpenGL wrappers and renderers. Shaders are always embedded into the binary;
# Debug builds additionally read them from the source tree so they can be hot-reloaded.
pfl_embed_resources(pfl_shaders
    BASE_DIR "${PROJECT_SOURCE_DIR}"
    FILES
        shaders/particles.vert
        shaders/particles.frag
        shaders/fullscreen.vert
        shaders/fade.frag)

add_library(pfl_render STATIC
    gl.hpp
    shader.hpp shader.cpp
    shader_sources.hpp shader_sources.cpp
    gl_objects.hpp gl_objects.cpp
    framebuffer.hpp framebuffer.cpp
    particle_renderer.hpp particle_renderer.cpp
    trail_renderer.hpp trail_renderer.cpp
    gl_debug.hpp gl_debug.cpp)
target_include_directories(pfl_render PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(pfl_render PUBLIC pfl_core glad PRIVATE pfl_shaders)
if(PFL_SHADERS_FROM_DISK)
    target_compile_definitions(pfl_render PRIVATE
        "$<$<CONFIG:Debug>:PFL_SHADER_DIR=\"${PROJECT_SOURCE_DIR}/shaders\">")
endif()
pfl_set_warnings(pfl_render)
