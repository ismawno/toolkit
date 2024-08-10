cmake_minimum_required(VERSION 3.16)
project(toolkit)

# Source files for the library
set(SOURCES
    src/logging/logging.cpp
    src/core/pch.cpp
    src/memory/memory.cpp
    src/profiling/clock.cpp
    src/profiling/profiler.cpp
    src/profiling/timespan.cpp
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

target_link_libraries(toolkit PRIVATE atomic)

# Include directories for the library
target_include_directories(toolkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_precompile_headers(toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/pch.hpp)
target_compile_definitions(toolkit PUBLIC KIT_ROOT_PATH="${TOOLKIT_ROOT_PATH}")

if(TOOLKIT_ENABLE_INFO_LOGS)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_INFO_LOGS)
endif()

if(TOOLKIT_ENABLE_WARNING_LOGS)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_WARNING_LOGS)
endif()

if(TOOLKIT_ENABLE_ASSERTS)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_ASSERTS)
endif()

if (TOOLKIT_ENABLE_LOG_COLORS)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_LOG_COLORS)
endif()

if (TOOLKIT_DISABLE_BLOCK_ALLOCATOR)
    target_compile_definitions(toolkit PUBLIC KIT_DISABLE_BLOCK_ALLOCATOR)
endif()

if (TOOLKIT_BLOCK_ALLOCATOR_THREAD_SAFE)
    target_compile_definitions(toolkit PUBLIC KIT_BLOCK_ALLOCATOR_THREAD_SAFE)
endif()