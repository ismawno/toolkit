cmake_minimum_required(VERSION 3.16)
project(toolkit-tests)

set(NAME toolkit-tests)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0)

FetchContent_MakeAvailable(Catch2)

set(SOURCES
    tests/memory/block_allocator.cpp
    tests/memory/stack_allocator.cpp
    tests/memory/arena_allocator.cpp
    tests/memory/ptr.cpp
    tests/container/static_array.cpp
    tests/container/dynamic_array.cpp
    tests/container/container.cpp
    tests/container/weak_array.cpp
    tests/container/span.cpp
    tests/container/storage.cpp
    tests/multiprocessing/task.cpp
    tests/multiprocessing/thread_pool.cpp
    tests/multiprocessing/for_each.cpp
    tests/utils/result.cpp
    tests/utils/hash.cpp)

add_executable(toolkit-tests ${SOURCES})

target_link_libraries(toolkit-tests PRIVATE Catch2::Catch2WithMain toolkit)
target_include_directories(toolkit-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

tkit_default_configure(toolkit-tests)

include(CTest)
include(Catch)
catch_discover_tests(toolkit-tests)
