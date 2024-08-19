cmake_minimum_required(VERSION 3.16)
project(toolkit)

# Source files for the library
set(SOURCES
    src/core/logging.cpp
    src/core/pch.cpp
    src/memory/memory.cpp
    src/memory/stack_allocator.cpp
    src/profiling/clock.cpp
    src/profiling/profiler.cpp
    src/profiling/timespan.cpp
    src/multiprocessing/task.cpp
    src/multiprocessing/task_manager.cpp
    src/multiprocessing/thread_pool.cpp
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
target_compile_definitions(toolkit PUBLIC KIT_ROOT_PATH="${TOOLKIT_ROOT_PATH}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(toolkit PUBLIC KIT_DEBUG)
    if (NOT TOOLKIT_ENABLE_INFO_LOGS)
        message(WARNING "Info logs are disabled in a debug build. This is not recommended")
    endif()
    if (NOT TOOLKIT_ENABLE_WARNING_LOGS)
        message(WARNING "Warning logs are disabled in a debug build. This is not recommended")
    endif()
    if (NOT TOOLKIT_ENABLE_ASSERTS)
        message(WARNING "Asserts are disabled in a debug build. This is not recommended")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(toolkit PUBLIC KIT_RELEASE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
    target_compile_definitions(toolkit PUBLIC KIT_DIST)
endif()

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

if (TOOLKIT_SILENT_ASSERTS)
    target_compile_definitions(toolkit PUBLIC KIT_SILENT_ASSERTS)
endif()

if (TOOLKIT_ENABLE_EXCEPTIONS)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_EXCEPTIONS)
endif()

if (TOOLKIT_ENABLE_BLOCK_ALLOCATOR)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_BLOCK_ALLOCATOR)
endif()