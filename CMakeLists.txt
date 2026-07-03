cmake_minimum_required(VERSION 3.21)

project(anvil LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH
    "Directory for built libraries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH
    "Directory for built executables")

option(ANVIL_BUILD_TESTS "Build unit tests" OFF)

message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir: ${CMAKE_BINARY_DIR}")
message(STATUS "CXX compiler:
    ${CMAKE_CXX_COMPILER}
    (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION})"
)
message(STATUS "CXX standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Export compile commands: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
message(STATUS "Build tests: ${ANVIL_BUILD_TESTS}")

if(CMAKE_CXX_COMPILER_TARGET)
    message(STATUS "CXX compiler target: ${CMAKE_CXX_COMPILER_TARGET}")
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(COMPILE_COMMANDS_LINK "${CMAKE_SOURCE_DIR}/compile_commands.json")
    if(NOT EXISTS "${COMPILE_COMMANDS_LINK}")
        file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json" "${COMPILE_COMMANDS_LINK}" SYMBOLIC)
        message(STATUS "Created compile_commands.json symlink at: ${COMPILE_COMMANDS_LINK}")
    elseif(IS_SYMLINK "${COMPILE_COMMANDS_LINK}")
        message(STATUS "compile_commands.json symlink already present: ${COMPILE_COMMANDS_LINK}")
    else()
        message(WARNING "${COMPILE_COMMANDS_LINK} exists and is not a symlink; clangd may use stale commands")
    endif()

    message(STATUS "compile_commands.json path: ${CMAKE_BINARY_DIR}/compile_commands.json")
endif()

add_subdirectory(src)

if(ANVIL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
