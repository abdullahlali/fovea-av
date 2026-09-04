add_executable(fovea_tests
    test_smoke.cpp
)

target_link_libraries(fovea_tests PRIVATE fovea_core)

add_test(NAME fovea_smoke COMMAND fovea_tests)
