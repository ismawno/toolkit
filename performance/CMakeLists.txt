cmake_minimum_required(VERSION 3.16)
project(toolkit-performance)

include(FetchContent)

set(OLD_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}} -Wno-error")

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
)

FetchContent_MakeAvailable(yaml-cpp)

# yaml-cpp has warnings, so we need to disable -Werror...


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../toolkit/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/memory.cpp
    src/multiprocessing.cpp
    src/settings.cpp
)

add_executable(toolkit-performance ${SOURCES})
target_link_libraries(toolkit-performance PRIVATE toolkit yaml-cpp)

set(CMAKE_CXX_FLAGS "${OLD_CXX_FLAGS}")