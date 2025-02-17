cmake_minimum_required(VERSION 3.16)
project(toolkit-performance)

set(NAME toolkit-performance)



# yaml-cpp has warnings, so I need to disable -Werror...

set(SOURCES
    perf/main.cpp
    perf/memory.cpp
    perf/multiprocessing.cpp
    perf/container.cpp
    perf/settings.cpp
)

add_executable(toolkit-performance ${SOURCES})
target_link_libraries(toolkit-performance PRIVATE toolkit)
target_include_directories(toolkit-performance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

tkit_configure_compile_link_options(toolkit-performance)