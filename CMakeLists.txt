cmake_minimum_required(VERSION 3.14)
project(vamp_wavediff_suite
    VERSION 0.2.0
    DESCRIPTION "Vamp plugin + Qt UI for A/B wave comparison"
    LANGUAGES CXX)

option(WAVEDIFF_BUILD_PLUGIN "Build the Vamp plugin"        ON)
option(WAVEDIFF_BUILD_UI     "Build the Qt Quick frontend"  ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Quality-of-life defaults for sub-projects.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WAVEDIFF_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()

if(WAVEDIFF_BUILD_UI)
    add_subdirectory(ui)
endif()

include(GNUInstallDirs)
install(PROGRAMS scripts/merger.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME wavediff-merger)
