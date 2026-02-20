set(EPIGIMP_APP_TEST_SOURCES
        test_AppService.cpp
        test_History.cpp
        test_BucketFill.cpp
)

add_executable(test_app
        ${EPIGIMP_APP_TEST_SOURCES}
)

target_include_directories(test_app
        PRIVATE
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(test_app
        PRIVATE
        epigimp_app
        epigimp_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_app)
