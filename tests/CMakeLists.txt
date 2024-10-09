cmake_minimum_required(VERSION 3.16)
project(toolkit-tests)

set(NAME toolkit-tests)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.6.0
)

FetchContent_MakeAvailable(Catch2)

set(SOURCES
    src/memory/block_allocator.cpp
    src/memory/stack_allocator.cpp
    src/memory/ptr.cpp
    src/container/static_array.cpp
    src/container/array.cpp
    src/container/hashable_tuple.cpp
    src/container/storage.cpp
    src/multiprocessing/thread_pool.cpp
)

add_executable(toolkit-tests ${SOURCES})

target_link_libraries(toolkit-tests PRIVATE Catch2::Catch2WithMain toolkit)
target_include_directories(toolkit-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (MSVC)
    configure_compile_link_options(toolkit-tests "/W4 /WX")
else()
    configure_compile_link_options(toolkit-tests "-Wall -Wextra -Wpedantic -Werror")
endif()

message(STATUS "TOOLKIT TESTS: The following options are overriden by tests:")
message(STATUS "TOOLKIT TESTS: Enable asserts: ON")
message(STATUS "TOOLKIT TESTS: Enable warning logs: ON")
message(STATUS "TOOLKIT TESTS: Enable info logs: ON")
message(STATUS "TOOLKIT TESTS: Silent asserts: ON")
message(STATUS "TOOLKIT TESTS: Enable exceptions: ON")

# These MUST be on for toolkit-tests to work properly
target_compile_definitions(toolkit PUBLIC KIT_ENABLE_WARNING_LOGS)
target_compile_definitions(toolkit PUBLIC KIT_ENABLE_INFO_LOGS)
target_compile_definitions(toolkit PUBLIC KIT_ENABLE_ASSERTS)
target_compile_definitions(toolkit PUBLIC KIT_SILENT_ASSERTS)
target_compile_definitions(toolkit PUBLIC KIT_ENABLE_BLOCK_ALLOCATOR)

include(CTest)
include(Catch)
Catch_discover_tests(toolkit-tests)
