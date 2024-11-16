cmake_minimum_required(VERSION 3.16)
project(toolkit)

set(NAME toolkit)

# Source files for the library
set(SOURCES
    kit/core/logging.cpp
    kit/core/pch.cpp
    kit/memory/memory.cpp
    kit/memory/stack_allocator.cpp
    kit/profiling/clock.cpp
    kit/profiling/timespan.cpp
    kit/multiprocessing/task.cpp
    kit/multiprocessing/task_manager.cpp
    kit/multiprocessing/thread_pool.cpp
    kit/multiprocessing/spin_lock.cpp
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
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_PROFILING)
    
    add_subdirectory(${tracy_SOURCE_DIR}/profiler ${CMAKE_BINARY_DIR}/profiler)

    if (MSVC)
        target_compile_options(tracy-profiler PRIVATE /wd4996)
        target_compile_options(TracyClient PRIVATE /wd4996)
        target_compile_options(TracyServer PRIVATE /wd4996)
    else()
        target_compile_options(tracy-profiler PRIVATE -Wno-deprecated-declarations)
        target_compile_options(TracyClient PRIVATE -Wno-deprecated-declarations)
        target_compile_options(TracyServer PRIVATE -Wno-deprecated-declarations)
        target_compile_options(tracy-profiler PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
        target_compile_options(TracyClient PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
        target_compile_options(TracyServer PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
    endif()

    if (TOOLKIT_ENABLE_VULKAN_PROFILING)
        target_compile_definitions(toolkit PUBLIC KIT_ENABLE_VULKAN_PROFILING)
    endif()

endif()

target_include_directories(toolkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/kit/core/pch.hpp)

if (MSVC)
    configure_compile_link_options(toolkit "/W4 /WX")
else()
    configure_compile_link_options(toolkit "-Wall -Wextra -Wpedantic -Werror")
endif()

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

if (TOOLKIT_SILENT_ASSERTS)
    target_compile_definitions(toolkit PUBLIC KIT_SILENT_ASSERTS)
endif()

if (TOOLKIT_WEAK_ASSERTS)
    target_compile_definitions(toolkit PUBLIC KIT_WEAK_ASSERTS)
endif()

if (TOOLKIT_ENABLE_BLOCK_ALLOCATOR)
    target_compile_definitions(toolkit PUBLIC KIT_ENABLE_BLOCK_ALLOCATOR)
endif()