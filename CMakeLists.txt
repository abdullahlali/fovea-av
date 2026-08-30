cmake_minimum_required(VERSION 3.20)
project(fovea VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE AND EXISTS "/opt/homebrew/opt/qt/lib/cmake/Qt6")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
endif()

option(FOVEA_BUILD_TESTS "Build unit tests" OFF)
option(FOVEA_ENABLE_QT "Build Qt visualization shell" ON)
option(FOVEA_ENABLE_ONNX "Enable ONNX Runtime inference" ON)
option(FOVEA_ENABLE_VIDEO "Enable OpenCV video replay" ON)

add_subdirectory(core)

if(FOVEA_ENABLE_QT)
    add_subdirectory(app)
endif()

if(FOVEA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
