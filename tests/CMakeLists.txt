enable_testing()
find_package(GTest REQUIRED)

add_executable(cachy_dock_tests
    test_parabolic_engine.cpp
    test_bounce_physics.cpp
    test_autohide_statemachine.cpp
    test_xdg_desktop_repository.cpp
)

target_link_libraries(cachy_dock_tests PRIVATE
    cachy_dock_core
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME AllUnitTests COMMAND cachy_dock_tests)
