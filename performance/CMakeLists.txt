cmake_minimum_required(VERSION 3.16)
project(toolkit-performance)

set(NAME toolkit-performance)

set(SOURCES perf/main.cpp perf/memory.cpp perf/multiprocessing.cpp
            perf/container.cpp perf/settings.cpp)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/ismawno/argparse.git
  GIT_TAG v3.2.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(argparse)

add_executable(toolkit-performance ${SOURCES})
tkit_register_for_yaml_serialization(toolkit-performance SOURCES
                                     perf/settings.hpp)
tkit_register_for_reflection(toolkit-performance SOURCES perf/settings.hpp)

target_link_libraries(toolkit-performance PRIVATE toolkit argparse)
target_include_directories(toolkit-performance
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

tkit_configure_compile_link_options(toolkit-performance NO_EXCEPTIONS)
