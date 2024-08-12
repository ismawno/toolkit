cmake_minimum_required(VERSION 3.16)
project(toolkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build types" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
option(TOOLKIT_ENABLE_EXCEPTIONS "Enable exceptions" OFF)
option(TOOLKIT_ENABLE_BLOCK_ALLOCATOR "Enable block allocations" ON)

if (TOOLKIT_BUILD_PERFORMANCE AND (NOT CMAKE_BUILD_TYPE STREQUAL "Dist"))
    message(WARNING "Building benchamrks in a non-distribution configuration! Results may not be representative")
endif()

if (TOOLKIT_BUILD_PERFORMANCE AND (TOOLKIT_ENABLE_INFO_LOGS OR TOOLKIT_ENABLE_WARNING_LOGS OR TOOLKIT_ENABLE_ASSERTS))
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
message(STATUS "TOOLKIT: Enable exceptions: ${TOOLKIT_ENABLE_EXCEPTIONS}")
message(STATUS "TOOLKIT: Enable block allocator: ${TOOLKIT_ENABLE_BLOCK_ALLOCATOR}")


# Save options to cache (currently commented out because it just annoys me)
# set(TOOLKIT_BUILD_TESTS ${TOOLKIT_BUILD_TESTS} CACHE BOOL "Build the tests")
# set(TOOLKIT_ENABLE_INFO_LOGS ${TOOLKIT_ENABLE_INFO_LOGS} CACHE BOOL "Enable info logs")
# set(TOOLKIT_ENABLE_WARNING_LOGS ${TOOLKIT_ENABLE_WARNING_LOGS} CACHE BOOL "Enable warning logs")
# set(TOOLKIT_ENABLE_ASSERTS ${TOOLKIT_ENABLE_ASSERTS} CACHE BOOL "Enable asserts")
# set(TOOLKIT_ENABLE_LOG_COLORS ${TOOLKIT_ENABLE_LOG_COLORS} CACHE BOOL "Enable log colors")

# Define linker flags for custom configurations. I dont really know what I'm doing here
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING "Linker flags for Debug configuration")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Linker flags for Release configuration")
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_DIST}" CACHE STRING "Linker flags for Dist configuration")

set(TOOLKIT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Root path to be used")

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")
    set(CMAKE_C_FLAGS_DEBUG "/Zi /Od")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/Zi /O2")
    set(CMAKE_C_FLAGS_RELEASE "/Zi /O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O2")
    set(CMAKE_C_FLAGS_RELEASE "-g -O2")
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS_DIST "/Ox")
    set(CMAKE_C_FLAGS_DIST "/Ox")
else()
    set(CMAKE_CXX_FLAGS_DIST "-O3")
    set(CMAKE_C_FLAGS_DIST "-O3")
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}} /W4 /WX /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}} /W4 /WX /MP")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}} -Wall -Wextra -Werror")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}} -Wall -Wextra -Werror")
endif()

# Add subdirectories
add_subdirectory(toolkit)

if (TOOLKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (TOOLKIT_BUILD_PERFORMANCE)
    add_subdirectory(performance)
endif()