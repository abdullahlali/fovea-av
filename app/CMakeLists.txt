find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

set(FOVEA_APP_SOURCES
    main.cpp
    scene_window.cpp
)

set(FOVEA_APP_HEADERS
    scene_window.hpp
)

qt_add_executable(fovea_app ${FOVEA_APP_SOURCES} ${FOVEA_APP_HEADERS})
target_link_libraries(fovea_app PRIVATE Qt6::Widgets fovea_core)

set_target_properties(fovea_app PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)
