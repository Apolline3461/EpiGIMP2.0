# tests/ui/CMakeLists.txt

set(EPIGIMP_UI_TEST_SOURCES
        test_MainWindow.cpp
        test_ImageConversion.cpp
        test_pan_clamp.cpp
)

add_executable(test_ui
        ${EPIGIMP_UI_TEST_SOURCES}
)

target_include_directories(test_ui
        PRIVATE
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(test_ui
        PRIVATE
        epigimp_ui
        Qt6::Widgets
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_ui)
