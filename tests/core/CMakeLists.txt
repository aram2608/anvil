add_executable(core_tests
    lexer_test.cpp
    parser_test.cpp
    compiler_test.cpp
    vm_test.cpp
)

target_link_libraries(core_tests PRIVATE core utils GTest::gtest_main)

gtest_discover_tests(core_tests)
