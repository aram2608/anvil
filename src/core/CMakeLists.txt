add_library(core STATIC
    lexer.cpp
    parser.cpp
    ast.cpp
)

target_include_directories(core PUBLIC ${INCLUDE_DIR})

