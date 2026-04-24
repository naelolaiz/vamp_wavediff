cmake_minimum_required(VERSION 3.14)
project(vamp_wavediff
    VERSION 0.2.0
    DESCRIPTION "Vamp plugin for A/B audio comparison"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Locate the Vamp SDK. Prefer pkg-config; fall back to VAMP_SDK_PATH for
# non-standard installations.
find_package(PkgConfig QUIET)
set(_vamp_found FALSE)

if(PkgConfig_FOUND)
    pkg_check_modules(VAMP_SDK IMPORTED_TARGET QUIET vamp-sdk)
    if(VAMP_SDK_FOUND)
        set(_vamp_found TRUE)
    endif()
endif()

if(NOT _vamp_found)
    set(VAMP_SDK_PATH "" CACHE PATH "Path to the Vamp SDK installation root")
    if(NOT VAMP_SDK_PATH)
        message(FATAL_ERROR
            "Vamp SDK not found via pkg-config. Set VAMP_SDK_PATH to its "
            "installation root (containing include/ and lib[64]/).")
    endif()

    find_path(VAMP_SDK_INCLUDE_DIR
        NAMES vamp-sdk/Plugin.h
        HINTS "${VAMP_SDK_PATH}/include"
        REQUIRED)

    find_library(VAMP_SDK_LIBRARY
        NAMES vamp-sdk
        HINTS "${VAMP_SDK_PATH}/lib" "${VAMP_SDK_PATH}/lib64"
        REQUIRED)

    add_library(vamp-sdk-imported UNKNOWN IMPORTED)
    set_target_properties(vamp-sdk-imported PROPERTIES
        IMPORTED_LOCATION "${VAMP_SDK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${VAMP_SDK_INCLUDE_DIR}")
endif()

add_library(${PROJECT_NAME} MODULE
    plugins/plugins.cpp
    plugins/WaveDiffPlugin.cpp
    plugins/WaveDiffPlugin.h)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

if(_vamp_found)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::VAMP_SDK)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE vamp-sdk-imported)
endif()

# Only export the symbol the Vamp host expects.
if(UNIX AND NOT APPLE)
    set(_vamp_version_script "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin.map")
    file(WRITE "${_vamp_version_script}"
        "{ global: vampGetPluginDescriptor; local: *; };\n")
    target_link_options(${PROJECT_NAME} PRIVATE
        "-Wl,--version-script=${_vamp_version_script}")
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/vamp
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/vamp)
