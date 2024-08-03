cmake_minimum_required(VERSION 3.16)
project(toolkit)

# Source files for the library
set(SOURCES
    src/core/logging.cpp
    src/core/pch.cpp
    src/core/memory/memory.cpp
)

# Create the library
add_library(toolkit STATIC ${SOURCES})

if (UNIX AND NOT APPLE)
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.2
    )
    FetchContent_MakeAvailable(fmt)
    target_link_libraries(toolkit PUBLIC fmt::fmt)
endif()

# Include directories for the library
target_include_directories(toolkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_precompile_headers(toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/pch.hpp)
target_compile_definitions(toolkit PUBLIC IBP_ROOT_PATH="${ROOT_PATH}")

if(ENABLE_INFO_LOGS)
    target_compile_definitions(toolkit PUBLIC IBP_ENABLE_INFO_LOGS)
endif()

if(ENABLE_WARNING_LOGS)
    target_compile_definitions(toolkit PUBLIC IBP_ENABLE_WARNING_LOGS)
endif()

if(ENABLE_ASSERTS)
    target_compile_definitions(toolkit PUBLIC IBP_ENABLE_ASSERTS)
endif()

if (ENABLE_LOG_COLORS)
    target_compile_definitions(toolkit PUBLIC IBP_ENABLE_LOG_COLORS)
endif()
