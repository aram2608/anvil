add_library(utils STATIC
    soa.cpp
)

target_include_directories(utils PUBLIC ${INCLUDE_DIR})

