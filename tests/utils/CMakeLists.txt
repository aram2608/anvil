add_executable(utils_test
    strpool_test.cpp
)

target_link_libraries(utils_test PRIVATE utils GTest::gtest_main)
gtest_discover_tests(utils_test)
