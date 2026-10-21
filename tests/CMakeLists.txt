# pfl_tests: doctest unit tests. Links only pfl_core, which proves core builds without graphics.
add_executable(pfl_tests test_main.cpp test_core.cpp)
target_link_libraries(pfl_tests PRIVATE pfl_core doctest::doctest)
pfl_set_warnings(pfl_tests)
add_test(NAME pfl_tests COMMAND pfl_tests)
