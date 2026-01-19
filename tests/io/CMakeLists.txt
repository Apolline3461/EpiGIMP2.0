set(EPIGIMP_IO_TEST_SOURCES test_Epg.cpp)

    add_executable(test_io ${EPIGIMP_IO_TEST_SOURCES})

        target_include_directories(test_io PRIVATE ${EPIGIMP_ROOT_INCLUDE_DIR})

            target_link_libraries(test_io PRIVATE epigimp_io GTest::gtest_main stb_headers)

                include(GoogleTest) gtest_discover_tests(test_io)