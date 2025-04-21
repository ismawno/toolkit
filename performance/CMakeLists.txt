cmake_minimum_required(VERSION 3.16)
project(toolkit-performance)

set(NAME toolkit-performance)

set(SOURCES
    perf/main.cpp
    perf/memory.cpp
    perf/multiprocessing.cpp
    perf/container.cpp
    perf/settings.cpp
)

add_executable(toolkit-performance ${SOURCES})
tkit_register_for_reflection(toolkit-performance SOURCES perf/settings.hpp)

target_link_libraries(toolkit-performance PRIVATE toolkit)
target_include_directories(toolkit-performance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

tkit_configure_compile_link_options(toolkit-performance)