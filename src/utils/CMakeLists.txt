add_library(utils STATIC
    soa.cpp
    strpool.cpp
)

target_include_directories(utils PUBLIC ${INCLUDE_DIR})

