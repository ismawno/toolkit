cmake_minimum_required(VERSION 3.16)
project(toolkit-tests)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.6.0
)

FetchContent_MakeAvailable(Catch2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../toolkit/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
    memory/block_allocator.cpp
    container/static_array.cpp
)

add_executable(toolkit-tests ${SOURCES})
target_link_libraries(toolkit-tests PRIVATE Catch2::Catch2WithMain toolkit)

# These MUST be on for toolkit-tests to work properly
target_compile_definitions(toolkit PUBLIC KIT_THROW)
target_compile_definitions(toolkit PUBLIC KIT_ENABLE_WARNING_LOGS)
target_compile_definitions(toolkit PUBLIC KIT_ENABLE_ASSERTS)
target_compile_definitions(toolkit PUBLIC KIT_SILENT_ASSERTS)

include(CTest)
include(Catch)
Catch_discover_tests(toolkit-tests)
add_test(NAME toolkit-toolkit-tests COMMAND toolkit-tests)