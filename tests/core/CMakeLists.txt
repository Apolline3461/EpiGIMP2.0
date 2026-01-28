set(EPIGIMP_CORE_TEST_SOURCES
        test_ImageBuffer.cpp
        test_Layer.cpp
        test_Document.cpp
        test_Compositor.cpp
        test_BucketFill.cpp
        test_BucketFill_benchmark.cpp
)

add_executable(test_core
        ${EPIGIMP_CORE_TEST_SOURCES}
)

target_include_directories(test_core
        PRIVATE
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(test_core
        PRIVATE
        epigimp_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_core)