include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(core_tests
    core/lexer_test.cpp
    core/parser_test.cpp
)

target_link_libraries(core_tests PRIVATE core utils GTest::gtest_main)

gtest_discover_tests(core_tests)
