# Script mode (cmake -P), invoked by the `dist` target in Dist.cmake.
if(NOT CONFIG STREQUAL "Release")
    message(FATAL_ERROR "dist must be built from the Release configuration "
                        "(cmake --build --preset release --target dist), got '${CONFIG}'.")
endif()

file(REMOVE_RECURSE "${STAGE}")
file(MAKE_DIRECTORY "${STAGE}")
file(COPY_FILE "${EXE}" "${STAGE}/PotentialFlowLab.exe")
file(COPY_FILE "${README}" "${STAGE}/README.txt")

cmake_path(GET OUT PARENT_PATH out_dir)
file(MAKE_DIRECTORY "${out_dir}")
file(REMOVE "${OUT}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${OUT}" --format=zip PotentialFlowLab.exe README.txt
    WORKING_DIRECTORY "${STAGE}"
    COMMAND_ERROR_IS_FATAL ANY)
message(STATUS "Wrote ${OUT}")
