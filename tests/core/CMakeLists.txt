add_executable(test_core_imagebuffer
        test_ImageBuffer.cpp
)

target_include_directories(test_core_imagebuffer
        PRIVATE
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(test_core_imagebuffer
        PRIVATE
        epigimp_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_core_imagebuffer)