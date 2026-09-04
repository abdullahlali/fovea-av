find_package(Qt6 REQUIRED COMPONENTS Widgets WebSockets)

find_package(Qt6 COMPONENTS WebEngineWidgets QUIET)
if(Qt6WebEngineWidgets_FOUND)
    message(STATUS "Fovea: Qt WebEngine enabled (embedded passenger panel)")
    set(FOVEA_HAS_WEBENGINE 1)
else()
    message(STATUS "Fovea: Qt WebEngine not found — cabin opens external panel URL")
endif()

qt_standard_project_setup()

set(FOVEA_APP_SOURCES
    main.cpp
    scene_window.cpp
    panel_bridge.cpp
    cabin_window.cpp
    panel_static_server.cpp
    panel_engine.cpp
    panel_window.cpp
)

set(FOVEA_APP_HEADERS
    scene_window.hpp
    panel_bridge.hpp
    cabin_window.hpp
    panel_static_server.hpp
    panel_engine.hpp
    panel_window.hpp
)

qt_add_executable(fovea_app ${FOVEA_APP_SOURCES} ${FOVEA_APP_HEADERS} video_window.cpp video_window.hpp)
target_link_libraries(fovea_app PRIVATE Qt6::Widgets Qt6::WebSockets fovea_core)

if(FOVEA_HAS_WEBENGINE)
    target_link_libraries(fovea_app PRIVATE Qt6::WebEngineWidgets)
    target_compile_definitions(fovea_app PRIVATE FOVEA_HAS_WEBENGINE)
endif()

set_target_properties(fovea_app PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)
