cmake_minimum_required(VERSION 3.20)
project(fovea VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FOVEA_BUILD_TESTS "Build unit tests" OFF)
option(FOVEA_ENABLE_QT "Build Qt visualization shell" OFF)
option(FOVEA_ENABLE_ONNX "Enable ONNX Runtime inference" ON)

add_subdirectory(core)

if(FOVEA_ENABLE_QT)
    add_subdirectory(app)
endif()

if(FOVEA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
