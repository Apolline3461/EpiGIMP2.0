set(EPIGIMP_UI_TEST_SOURCES
        test_ImageConversion.cpp
)

add_executable(test_ui
        ${EPIGIMP_UI_TEST_SOURCES}
        ${CMAKE_SOURCE_DIR}/src/ui/ImageConversion.cpp
)

target_include_directories(test_ui
        PRIVATE
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(test_ui
        PRIVATE
        epigimp_core
        Qt6::Gui
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_ui)
