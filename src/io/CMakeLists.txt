# pfl_io: scene save/load (JSON). Placeholder for now; linked so nlohmann/json is proven to build.
add_library(pfl_io STATIC scene_io.hpp scene_io.cpp)
target_include_directories(pfl_io PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(pfl_io PUBLIC pfl_core PRIVATE nlohmann_json::nlohmann_json)
pfl_set_warnings(pfl_io)
