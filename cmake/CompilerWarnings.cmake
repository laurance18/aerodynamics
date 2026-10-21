# Warning/conformance flags for our own targets only (dependencies keep their defaults).
function(pfl_set_warnings target)
    target_compile_options(${target} PRIVATE
        /W4 /permissive- /utf-8 /Zc:__cplusplus /Zc:preprocessor /external:W0)
    target_compile_definitions(${target} PRIVATE
        NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
endfunction()
