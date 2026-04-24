cmake_minimum_required(VERSION 3.11)
project(vamp_diff_plugins)

set(VAMP_SDK_PATH "" CACHE PATH "Path to the VAMP plugin SDK")
if(NOT VAMP_SDK_PATH)
    message(FATAL_ERROR " VAMP_SDK_PATH not set")
endif()

include_directories(${VAMP_SDK_PATH}/include)

add_library(${PROJECT_NAME} MODULE
    plugins/plugins.cpp
    plugins/FixedTempoEstimator.cpp
    plugins/PowerSpectrum.cpp
    plugins/WaveDiff.cpp
    plugins/ZeroCrossing.cpp
    plugins/FixedTempoEstimator.h
    plugins/PowerSpectrum.h
    plugins/WaveDiff.h
    plugins/ZeroCrossing.h
)

target_link_libraries(${PROJECT_NAME} ${VAMP_SDK_PATH}/lib64/libvamp-sdk.so)
