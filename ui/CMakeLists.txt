cmake_minimum_required(VERSION 3.14)
project(wavediff_ui
    VERSION 0.2.0
    DESCRIPTION "Qt/QML frontend for the vamp_wavediff plugin"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 COMPONENTS Core Gui Qml Quick QuickControls2 REQUIRED)

qt5_add_resources(WAVEDIFF_QT_RESOURCES qml.qrc)

add_executable(wavediff
    main.cpp
    Backend.cpp
    Backend.h
    ${WAVEDIFF_QT_RESOURCES})

target_compile_options(wavediff PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

target_compile_definitions(wavediff PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
    QT_DEPRECATED_WARNINGS)

target_link_libraries(wavediff PRIVATE
    Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick Qt5::QuickControls2)

include(GNUInstallDirs)
install(TARGETS wavediff RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
