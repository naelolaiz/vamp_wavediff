cmake_minimum_required(VERSION 3.14)

# The test binary compiles WaveDiffPlugin.cpp directly so we don't need to
# link against the MODULE the host loads. We re-run the same Vamp SDK
# discovery the plugin uses, scoped to this directory.

find_package(PkgConfig QUIET)
set(_vamp_found FALSE)

if(PkgConfig_FOUND)
    pkg_check_modules(VAMP_SDK_TESTS IMPORTED_TARGET QUIET vamp-sdk)
    if(VAMP_SDK_TESTS_FOUND)
        set(_vamp_found TRUE)
    endif()
endif()

if(NOT _vamp_found)
    if(NOT VAMP_SDK_PATH)
        message(FATAL_ERROR
            "Vamp SDK not found via pkg-config. Set VAMP_SDK_PATH to its "
            "installation root (containing include/ and lib[64]/).")
    endif()

    find_path(VAMP_SDK_TESTS_INCLUDE_DIR
        NAMES vamp-sdk/Plugin.h
        HINTS "${VAMP_SDK_PATH}/include"
        REQUIRED)

    find_library(VAMP_SDK_TESTS_LIBRARY
        NAMES vamp-sdk
        HINTS "${VAMP_SDK_PATH}/lib" "${VAMP_SDK_PATH}/lib64"
        REQUIRED)

    add_library(vamp-sdk-tests-imported UNKNOWN IMPORTED)
    set_target_properties(vamp-sdk-tests-imported PROPERTIES
        IMPORTED_LOCATION "${VAMP_SDK_TESTS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${VAMP_SDK_TESTS_INCLUDE_DIR}")
endif()

add_executable(test_wavediff
    test_wavediff.cpp
    ${CMAKE_SOURCE_DIR}/plugin/plugins/WaveDiffPlugin.cpp)

target_include_directories(test_wavediff PRIVATE
    ${CMAKE_SOURCE_DIR}/plugin/plugins)

target_compile_features(test_wavediff PRIVATE cxx_std_17)

target_compile_options(test_wavediff PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

if(_vamp_found)
    target_link_libraries(test_wavediff PRIVATE PkgConfig::VAMP_SDK_TESTS)
else()
    target_link_libraries(test_wavediff PRIVATE vamp-sdk-tests-imported)
endif()

add_test(NAME wavediff_unit_tests COMMAND test_wavediff)
