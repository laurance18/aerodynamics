# pfl_tests: doctest unit tests. Links only the graphics-free libraries (core, io).
add_executable(pfl_tests test_main.cpp test_core.cpp test_flow.cpp test_scene.cpp)
target_link_libraries(pfl_tests PRIVATE pfl_core pfl_io doctest::doctest)
pfl_set_warnings(pfl_tests)
add_test(NAME pfl_tests COMMAND pfl_tests)
target_compile_definitions(pfl_tests PRIVATE PFL_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/examples")
