add_library(core STATIC
    lexer.cpp
)

target_include_directories(core PUBLIC ${INCLUDE_DIR})

