add_library(core STATIC
    lexer.cpp
    parser.cpp
    ast.cpp
    compiler.cpp
    vm.cpp
    dis.cpp
    object.cpp
    strtable.cpp
)

target_include_directories(core PUBLIC ${INCLUDE_DIR})

