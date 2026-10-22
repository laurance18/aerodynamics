# pfl_ui: Dear ImGui / ImPlot panels. Talks to core types, never to OpenGL directly.
add_library(pfl_ui STATIC
    style.hpp style.cpp
    main_menu.hpp main_menu.cpp
    elements_menu.hpp elements_menu.cpp
    toolbar.hpp toolbar.cpp
    parameter_dialog.hpp parameter_dialog.cpp
    plot_view.hpp plot_view.cpp
    status_bar.hpp status_bar.cpp)
target_include_directories(pfl_ui PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(pfl_ui PUBLIC pfl_core pfl_imgui)
pfl_set_warnings(pfl_ui)
