# `dist` target: builds the app and writes dist/PotentialFlowLab.zip (exe + README.txt).
# Usage: cmake --build --preset release --target dist
configure_file("${PROJECT_SOURCE_DIR}/resources/dist_README.txt.in"
               "${CMAKE_BINARY_DIR}/dist_README.txt" @ONLY NEWLINE_STYLE CRLF)

add_custom_target(dist
    COMMAND "${CMAKE_COMMAND}"
            "-DCONFIG=$<CONFIG>"
            "-DEXE=$<TARGET_FILE:PotentialFlowLab>"
            "-DREADME=${CMAKE_BINARY_DIR}/dist_README.txt"
            "-DSTAGE=${CMAKE_BINARY_DIR}/dist_stage"
            "-DOUT=${PROJECT_SOURCE_DIR}/dist/PotentialFlowLab.zip"
            -P "${CMAKE_CURRENT_LIST_DIR}/package_zip.cmake"
    DEPENDS PotentialFlowLab
    COMMENT "Packaging dist/PotentialFlowLab.zip"
    VERBATIM)
