cmake_minimum_required(VERSION 3.16)
project(toolkit)

set(NAME toolkit)

# Source files for the library
set(SOURCES
    tkit/core/logging.cpp
    tkit/core/pch.cpp
    tkit/memory/memory.cpp
    tkit/memory/block_allocator.cpp
    tkit/memory/stack_allocator.cpp
    tkit/memory/arena_allocator.cpp
    tkit/profiling/clock.cpp
    tkit/profiling/timespan.cpp
    tkit/multiprocessing/task.cpp
    tkit/multiprocessing/task_manager.cpp
    tkit/multiprocessing/thread_pool.cpp
    tkit/multiprocessing/spin_lock.cpp
)

# Create the library
add_library(toolkit STATIC ${SOURCES})

include(FetchContent)
if (UNIX AND NOT APPLE)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.2
    )
    FetchContent_MakeAvailable(fmt)
    target_link_libraries(toolkit PUBLIC fmt::fmt)
endif()

if (TOOLKIT_ENABLE_PROFILING)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
    )
    FetchContent_MakeAvailable(tracy)
    target_link_libraries(toolkit PUBLIC TracyClient)
    target_compile_definitions(toolkit PUBLIC TRACY_ENABLE)
    target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_PROFILING)
    
    # Because of this check, the profiler binary will always persist and will keep the first cmake options that were set.
    # Recompile with other options by removing the profiler directory inside your build directory
    if (NOT EXISTS ${CMAKE_BINARY_DIR}/profiler)
        add_subdirectory(${tracy_SOURCE_DIR}/profiler ${CMAKE_BINARY_DIR}/profiler)
        if (MSVC)
            configure_compile_link_options(tracy-profiler FLAGS /wd4996 CONFIG Dist FORBID_SANITIZERS)
            configure_compile_link_options(TracyServer FLAGS /wd4996 CONFIG Dist FORBID_SANITIZERS)
        elseif(APPLE)
            configure_compile_link_options(tracy-profiler FLAGS -Wno-deprecated-declarations -Wno-deprecated-anon-enum-enum-conversion CONFIG Dist FORBID_SANITIZERS)
            configure_compile_link_options(TracyServer FLAGS -Wno-deprecated-declarations -Wno-deprecated-anon-enum-enum-conversion CONFIG Dist FORBID_SANITIZERS)
        else()
            configure_compile_link_options(tracy-profiler FLAGS -Wno-deprecated-declarations -Wno-deprecated-enum-enum-conversion CONFIG Dist FORBID_SANITIZERS)
            configure_compile_link_options(TracyServer FLAGS -Wno-deprecated-declarations -Wno-deprecated-enum-enum-conversion CONFIG Dist FORBID_SANITIZERS)
        endif()
    endif()

    if (MSVC)
        target_compile_options(TracyClient PRIVATE /wd4996)
    else()
        target_compile_options(TracyClient PRIVATE -Wno-deprecated-declarations)
        target_compile_options(TracyClient PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
    endif()

    if (TOOLKIT_ENABLE_VULKAN_PROFILING)
        target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_VULKAN_PROFILING)
    endif()

endif()

target_include_directories(toolkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tkit/core/pch.hpp)

if (MSVC)
    configure_compile_link_options(toolkit FLAGS /W4 /WX)
else()
    configure_compile_link_options(toolkit FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

target_compile_definitions(toolkit PUBLIC TKIT_ROOT_PATH="${TOOLKIT_ROOT_PATH}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(toolkit PUBLIC TKIT_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(toolkit PUBLIC TKIT_RELEASE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
    target_compile_definitions(toolkit PUBLIC TKIT_DIST)
endif()

if(TOOLKIT_ENABLE_INFO_LOGS)
    target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_INFO_LOGS)
endif()

if(TOOLKIT_ENABLE_WARNING_LOGS)
    target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_WARNING_LOGS)
endif()

if(TOOLKIT_ENABLE_ASSERTS)
    target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_ASSERTS)
endif()