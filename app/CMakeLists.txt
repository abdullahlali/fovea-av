find_package(Qt6 REQUIRED COMPONENTS Widgets WebSockets)

qt_standard_project_setup()

set(FOVEA_APP_SOURCES
    main.cpp
    scene_window.cpp
    panel_bridge.cpp
)

set(FOVEA_APP_HEADERS
    scene_window.hpp
    panel_bridge.hpp
)

qt_add_executable(fovea_app ${FOVEA_APP_SOURCES} ${FOVEA_APP_HEADERS} video_window.cpp video_window.hpp)
target_link_libraries(fovea_app PRIVATE Qt6::Widgets Qt6::WebSockets fovea_core)

set_target_properties(fovea_app PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)
