set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(core)
add_subdirectory(utils)

add_executable(anvil main.cpp)

target_compile_options(anvil
    PRIVATE
      -g
      $<$<CONFIG:Debug>:-Wall>
      $<$<CONFIG:Debug>:-Wextra>
      $<$<CONFIG:Debug>:-pedantic>
)

target_compile_definitions(anvil
    PRIVATE
      $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_include_directories(anvil PRIVATE ${INCLUDE_DIR})

target_link_libraries(anvil PRIVATE core utils)

