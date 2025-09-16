cmake_minimum_required(VERSION 3.16)
project(toolkit)

set(NAME toolkit)

set(SOURCES tkit/core/pch.cpp tkit/memory/memory.cpp)

if(TOOLKIT_ENABLE_INFO_LOGS
   OR TOOLKIT_ENABLE_WARNING_LOGS
   OR TOOLKIT_ENABLE_ASSERTS)
  list(APPEND SOURCES tkit/utils/logging.cpp)
endif()

if(TOOLKIT_ENABLE_ARENA_ALLOCATOR)
  list(APPEND SOURCES tkit/memory/arena_allocator.cpp)
endif()

if(TOOLKIT_ENABLE_BLOCK_ALLOCATOR)
  list(APPEND SOURCES tkit/memory/block_allocator.cpp)
endif()

if(TOOLKIT_ENABLE_STACK_ALLOCATOR)
  list(APPEND SOURCES tkit/memory/stack_allocator.cpp)
endif()

if(TOOLKIT_ENABLE_TIER_ALLOCATOR)
  list(APPEND SOURCES tkit/memory/tier_allocator.cpp)
endif()

if(TOOLKIT_ENABLE_MULTIPROCESSING)
  list(APPEND SOURCES tkit/multiprocessing/task.cpp
       tkit/multiprocessing/task_manager.cpp
       tkit/multiprocessing/thread_pool.cpp tkit/multiprocessing/topology.cpp)
endif()

if(TOOLKIT_ENABLE_PROFILING)
  list(APPEND SOURCES tkit/profiling/clock.cpp tkit/profiling/timespan.cpp)
endif()

include(FetchContent)
if(TOOLKIT_ENABLE_YAML_SERIALIZATION)

  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/ismawno/yaml-cpp.git
    GIT_TAG 0.8.x
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  set(YAML_CPP_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS
      OFF
      CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB
      OFF
      CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(yaml-cpp)

  tkit_configure_compile_link_options(yaml-cpp)
  list(APPEND SOURCES tkit/serialization/yaml/codec.cpp)
endif()

# Create the library
add_library(toolkit STATIC ${SOURCES})
target_compile_definitions(toolkit PUBLIC TKIT_VERSION=\"v0.5.0\")

if(TOOLKIT_ENABLE_YAML_SERIALIZATION)
  target_link_libraries(toolkit PUBLIC yaml-cpp)
endif()

if(TOOLKIT_ENABLE_MULTIPROCESSING)
  find_package(PkgConfig QUIET)

  if(PkgConfig_FOUND)
    pkg_check_modules(HWLOC QUIET IMPORTED_TARGET hwloc)
  endif()

  if(HWLOC_FOUND)

    message(STATUS "TOOLKIT: HWLOC found")
    target_link_libraries(toolkit PRIVATE PkgConfig::HWLOC)
    target_compile_definitions(toolkit PRIVATE TKIT_HWLOC_INSTALLED)

  else()
    message(STATUS "TOOLKIT: HWLOC not found")
  endif()
endif()

if(UNIX AND NOT APPLE)
  find_package(fmt)

  if(NOT TARGET fmt::fmt)
    message(STATUS "TOOLKIT: FMT not found. Fetching source...")
    FetchContent_Declare(
      fmt
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG 11.0.2
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE)

    FetchContent_MakeAvailable(fmt)
    tkit_configure_compile_link_options(fmt)
  else()
    message(STATUS "TOOLKIT: FMT found")
    # tkit_configure_compile_link_options(fmt::fmt)
  endif()

  target_link_libraries(toolkit PUBLIC fmt::fmt)
endif()

if(TOOLKIT_ENABLE_INSTRUMENTATION OR TOOLKIT_ENABLE_SAMPLING)
  if(TOOLKIT_ENABLE_INSTRUMENTATION)
    target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_INSTRUMENTATION)
  endif()
  if(TOOLKIT_ENABLE_SAMPLING)
    target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_SAMPLING)
  else()
    set(TRACY_NO_SAMPLING ON)
    if(NOT TOOLKIT_ENABLE_CALLSTACK)
      set(TRACY_NO_CALLSTACK ON)
    endif()
  endif()

  find_package(Tracy QUIET)

  if(NOT TARGET Tracy::TracyClient)
    set(TRACY_TARGET TracyClient)

    FetchContent_Declare(
      tracy
      GIT_REPOSITORY https://github.com/wolfpld/tracy.git
      GIT_TAG v0.12.2
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE)

    FetchContent_MakeAvailable(tracy)
  else()
    set(TRACY_TARGET Tracy::TracyClient)
  endif()
  target_link_libraries(toolkit PUBLIC ${TRACY_TARGET})
  target_compile_definitions(toolkit PUBLIC TRACY_ENABLE)
  target_compile_definitions(${TRACY_TARGET}
                             PUBLIC TRACY_SAMPLING_HZ=${TOOLKIT_SAMPLING_RATE})

  # Because of this check, the profiler binaries will always persist and will
  # keep the first cmake options that were set. Recompile with other options by
  # removing the profiler directory inside your build directory

  set(NO_STATISTICS
      OFF
      CACHE BOOL "I hate cmake" FORCE)

  if(MSVC)
    target_compile_options(${TRACY_TARGET} PRIVATE /wd4996)
  else()
    target_compile_options(${TRACY_TARGET} PRIVATE -Wno-deprecated-declarations)
    target_compile_options(${TRACY_TARGET}
                           PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
  endif()
  if(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION)
    target_compile_definitions(${TRACY_TARGET} PUBLIC TRACY_VK_USE_SYMBOL_TABLE)
    target_compile_definitions(toolkit
                               PUBLIC TKIT_ENABLE_VULKAN_INSTRUMENTATION)
  endif()

endif()

target_include_directories(toolkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(toolkit PRIVATE
                          ${CMAKE_CURRENT_SOURCE_DIR}/tkit/core/pch.hpp)

tkit_default_configure(toolkit)

target_compile_definitions(toolkit PUBLIC TKIT_ROOT_PATH="${TOOLKIT_ROOT_PATH}")

tkit_register_for_yaml_serialization(toolkit SOURCES tkit/utils/dimension.hpp)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(toolkit PUBLIC TKIT_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_definitions(toolkit PUBLIC TKIT_RELEASE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
  target_compile_definitions(toolkit PUBLIC TKIT_DIST)
endif()

if(TOOLKIT_ENABLE_ARENA_ALLOCATOR)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_ARENA_ALLOCATOR)
endif()

if(TOOLKIT_ENABLE_BLOCK_ALLOCATOR)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_BLOCK_ALLOCATOR)
endif()

if(TOOLKIT_ENABLE_STACK_ALLOCATOR)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_STACK_ALLOCATOR)
endif()

if(TOOLKIT_ENABLE_TIER_ALLOCATOR)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_TIER_ALLOCATOR)
endif()

if(TOOLKIT_ENABLE_MULTIPROCESSING)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_MULTIPROCESSING)
endif()

if(TOOLKIT_ENABLE_PROFILING)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_PROFILING)
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

if(TOOLKIT_ENABLE_REFLECTION)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_REFLECTION)
endif()

if(TOOLKIT_ENABLE_YAML_SERIALIZATION)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_YAML_SERIALIZATION)
endif()
