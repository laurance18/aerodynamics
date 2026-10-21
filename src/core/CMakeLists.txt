# pfl_core: pure math/physics. Must never depend on OpenGL, GLFW or ImGui.
add_library(pfl_core STATIC
    vec2.hpp
    dummy_flow.hpp dummy_flow.cpp
    particles.hpp particles.cpp
    flow_element.hpp flow_element.cpp
    flow_field.hpp flow_field.cpp
    streamline.hpp
    image_system.hpp)
target_include_directories(pfl_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
pfl_set_warnings(pfl_core)
