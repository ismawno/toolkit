cmake_minimum_required(VERSION 3.16)
project(toolkit-performance)

set(NAME toolkit-performance)

include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
)

FetchContent_MakeAvailable(yaml-cpp)

# yaml-cpp has warnings, so I need to disable -Werror...

set(SOURCES
    src/main.cpp
    src/memory.cpp
    src/multiprocessing.cpp
    src/settings.cpp
)

add_executable(toolkit-performance ${SOURCES})
target_link_libraries(toolkit-performance PRIVATE toolkit yaml-cpp)
target_include_directories(toolkit-performance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

configure_compile_link_options(toolkit-performance "")