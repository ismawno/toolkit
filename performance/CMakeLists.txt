cmake_minimum_required(VERSION 3.16)
project(toolkit-performance)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../toolkit/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
    main.cpp
)

add_executable(toolkit-performance ${SOURCES})
target_link_libraries(toolkit-performance PRIVATE toolkit)