set(FOVEA_CORE_SOURCES
    src/capture.cpp
    src/scene_graph.cpp
    src/predictor.cpp
    src/infer.cpp
    src/grok_client.cpp
    src/pipeline.cpp
    src/tracker.cpp
    src/camera_view.cpp
    src/scenario.cpp
    src/env.cpp
)

set(FOVEA_CORE_HEADERS
    include/fovea/capture.hpp
    include/fovea/scene_graph.hpp
    include/fovea/predictor.hpp
    include/fovea/infer.hpp
    include/fovea/grok_client.hpp
    include/fovea/pipeline.hpp
    include/fovea/types.hpp
    include/fovea/coco_labels.hpp
    include/fovea/tracker.hpp
    include/fovea/camera_view.hpp
    include/fovea/scenario.hpp
    include/fovea/env.hpp
)

add_library(fovea_core STATIC ${FOVEA_CORE_SOURCES} ${FOVEA_CORE_HEADERS})
target_include_directories(fovea_core PUBLIC include)
target_include_directories(fovea_core PRIVATE ${CMAKE_SOURCE_DIR}/third_party)

find_package(CURL REQUIRED)
target_link_libraries(fovea_core PUBLIC CURL::libcurl)

if(FOVEA_ENABLE_ONNX)
    find_package(onnxruntime CONFIG QUIET)
    if(NOT onnxruntime_FOUND)
        if(EXISTS "/opt/homebrew/opt/onnxruntime/lib/cmake/onnxruntime")
            set(onnxruntime_DIR "/opt/homebrew/opt/onnxruntime/lib/cmake/onnxruntime")
            find_package(onnxruntime CONFIG REQUIRED)
        endif()
    endif()

    if(onnxruntime_FOUND)
        target_sources(fovea_core PRIVATE src/infer_onnx.cpp)
        target_compile_definitions(fovea_core PUBLIC FOVEA_HAS_ONNX=1)
        target_link_libraries(fovea_core PUBLIC onnxruntime::onnxruntime)
        message(STATUS "Fovea: ONNX Runtime enabled")
    else()
        message(WARNING "ONNX Runtime not found. Building with stub inference.")
    endif()
endif()

if(FOVEA_ENABLE_VIDEO)
    find_package(OpenCV QUIET)
    if(OpenCV_FOUND)
        target_sources(fovea_core PRIVATE src/video_capture.cpp)
        target_sources(fovea_core PUBLIC include/fovea/video_capture.hpp)
        target_compile_definitions(fovea_core PUBLIC FOVEA_HAS_VIDEO=1)
        target_link_libraries(fovea_core PUBLIC ${OpenCV_LIBS})
        target_include_directories(fovea_core PUBLIC ${OpenCV_INCLUDE_DIRS})
        message(STATUS "Fovea: OpenCV video capture enabled")
    else()
        message(WARNING "OpenCV not found. Video replay disabled.")
    endif()
endif()

add_executable(fovea_cli src/main.cpp)
target_link_libraries(fovea_cli PRIVATE fovea_core)
