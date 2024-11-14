cmake_minimum_required(VERSION 3.16)
project(toolkit-root)

set(NAME toolkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build types" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TOOLKIT_SANITIZERS "OFF" CACHE STRING "Sanitizer(s) to use. Can be whichever one your compiler supports, and compatible ones may be included simultaneously (separated by commas). Set to OFF to disable")

if (CMAKE_BUILD_TYPE STREQUAL "Dist")
    option(TOOLKIT_BUILD_TESTS "Build the tests" OFF)
    option(TOOLKIT_BUILD_PERFORMANCE "Build performance tests" ON)
    option(TOOLKIT_ENABLE_INFO_LOGS "Enable info logs" OFF)
    option(TOOLKIT_ENABLE_WARNING_LOGS "Enable warning logs" OFF)
    option(TOOLKIT_ENABLE_ASSERTS "Enable asserts" OFF)
    option(TOOLKIT_ENABLE_LOG_COLORS "Enable log colors" OFF)
else()
    option(TOOLKIT_BUILD_TESTS "Build the tests" ON)
    option(TOOLKIT_BUILD_PERFORMANCE "Build performance tests" OFF)
    option(TOOLKIT_ENABLE_INFO_LOGS "Enable info logs" ON)
    option(TOOLKIT_ENABLE_WARNING_LOGS "Enable warning logs" ON)
    option(TOOLKIT_ENABLE_ASSERTS "Enable asserts" ON)
    option(TOOLKIT_ENABLE_LOG_COLORS "Enable log colors" ON)
endif()

option(TOOLKIT_SILENT_ASSERTS "Silence asserts" OFF)
option(TOOLKIT_ENABLE_BLOCK_ALLOCATOR "Enable block allocations" ON)
option(TOOLKIT_ENABLE_PROFILING "Enable profiling" OFF)

if (TOOLKIT_BUILD_PERFORMANCE AND (NOT CMAKE_BUILD_TYPE STREQUAL "Dist"))
    message(WARNING "Building benchamrks in a non-distribution configuration! Results may not be representative")
endif()

if (TOOLKIT_BUILD_PERFORMANCE AND (TOOLKIT_ENABLE_WARNING_LOGS OR TOOLKIT_ENABLE_ASSERTS))
    message(WARNING "Building benchamrks with asserts/logs enabled! Results may not be representative")
endif()

# Print all options
message(STATUS "TOOLKIT: Some of the following options may be overriden by other targets")
message(STATUS "TOOLKIT: Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "TOOLKIT: Build tests: ${TOOLKIT_BUILD_TESTS}")
message(STATUS "TOOLKIT: Build performance tests: ${TOOLKIT_BUILD_PERFORMANCE}")
message(STATUS "TOOLKIT: Enable info logs: ${TOOLKIT_ENABLE_INFO_LOGS}")
message(STATUS "TOOLKIT: Enable warning logs: ${TOOLKIT_ENABLE_WARNING_LOGS}")
message(STATUS "TOOLKIT: Enable asserts: ${TOOLKIT_ENABLE_ASSERTS}")
message(STATUS "TOOLKIT: Enable log colors: ${TOOLKIT_ENABLE_LOG_COLORS}")
message(STATUS "TOOLKIT: Silent asserts: ${TOOLKIT_SILENT_ASSERTS}")
message(STATUS "TOOLKIT: Enable block allocator: ${TOOLKIT_ENABLE_BLOCK_ALLOCATOR}")
message(STATUS "TOOLKIT: Sanitizer: ${TOOLKIT_SANITIZERS}")
message(STATUS "TOOLKIT: Profiling: ${TOOLKIT_ENABLE_PROFILING}")

set(TOOLKIT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Root path to be used")

# Define linker flags for custom configurations. CMake crashes in windows if this is missing
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING "Linker flags for Debug configuration")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Linker flags for Release configuration")
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_DIST}" CACHE STRING "Linker flags for Dist configuration")

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING "Linker flags for Debug configuration")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Linker flags for Release configuration")
set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS_DIST}" CACHE STRING "Linker flags for Dist configuration")

function(configure_compile_link_options target warning_flags)
    string(REPLACE " " ";" warning_flags_list "${warning_flags}")

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        if (MSVC)
            target_compile_options(${target} PRIVATE /ZI /Od /MP ${warning_flags_list})
            target_link_options(${target} PRIVATE /DEBUG /INCREMENTAL:NO)
        else()
            target_compile_options(${target} PRIVATE -g -O0 ${warning_flags_list})
        endif()

        # Some warnings for debug builds
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
        if (MSVC)
            target_compile_options(${target} PRIVATE /ZI /O2 /MP ${warning_flags_list})
            target_link_options(${target} PRIVATE /DEBUG /INCREMENTAL:NO)
        else()
            target_compile_options(${target} PRIVATE -g -O2 ${warning_flags_list})
        endif()

    elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
        if (MSVC)
            target_compile_options(${target} PRIVATE /Ox /GL /MP ${warning_flags_list})
            target_link_options(${target} PRIVATE /LTCG /INCREMENTAL:NO)
        else()
            target_compile_options(${target} PRIVATE -O3 -flto ${warning_flags_list})
            target_link_options(${target} PRIVATE -flto)
        endif()
    endif()

    if (TOOLKIT_SANITIZERS STREQUAL "OFF")
        return()
    endif()

    if (MSVC)
        target_compile_options(${target} PRIVATE /fsanitize=${TOOLKIT_SANITIZERS} /FS)
    else()
        target_compile_options(${target} PRIVATE -fsanitize=${TOOLKIT_SANITIZERS} -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=${TOOLKIT_SANITIZERS} -fno-omit-frame-pointer)
    endif()
endfunction()

# Add subdirectories
add_subdirectory(toolkit)

if (TOOLKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (TOOLKIT_BUILD_PERFORMANCE)
    add_subdirectory(performance)
endif()