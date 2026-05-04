cmake_minimum_required(VERSION 3.16)
project(wavediff_ui
    VERSION 0.2.0
    DESCRIPTION "Qt6/QML frontend for the vamp_wavediff plugin"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 COMPONENTS Core Gui Qml Quick QuickControls2 REQUIRED)

qt_add_executable(wavediff
    main.cpp)

qt_add_qml_module(wavediff
    URI WaveDiff
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        Backend.h
        Backend.cpp)

target_compile_options(wavediff PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

target_compile_definitions(wavediff PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(wavediff PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

include(GNUInstallDirs)
install(TARGETS wavediff
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
