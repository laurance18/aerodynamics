# pfl_ui: Dear ImGui / ImPlot panels. Talks to core types, never to OpenGL directly.
add_library(pfl_ui STATIC
    demo_panel.hpp demo_panel.cpp
    main_menu.hpp main_menu.cpp
    toolbar.hpp
    elements_menu.hpp
    parameter_dialog.hpp)
target_include_directories(pfl_ui PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(pfl_ui PUBLIC pfl_core pfl_imgui)
pfl_set_warnings(pfl_ui)
