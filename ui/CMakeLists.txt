cmake_minimum_required(VERSION 3.5)

project(WaveFileProcessor)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

qt5_add_resources(QT_RESOURCES qml.qrc)

add_executable(${PROJECT_NAME} "main.cpp" "Backend.cpp" ${QT_RESOURCES})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt5::Core Qt5::Quick)

