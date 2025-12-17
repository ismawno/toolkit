cmake_minimum_required(VERSION 3.16)
project(toolkit)

set(NAME toolkit)

set(SOURCES tkit/core/pch.cpp tkit/memory/memory.cpp tkit/utils/logging.cpp)

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
  list(APPEND SOURCES tkit/profiling/clock.cpp)
endif()

include(FetchContent)
if(TOOLKIT_ENABLE_YAML_SERIALIZATION)

  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/ismawno/yaml-cpp.git
    GIT_TAG v0.8.0
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

  tkit_configure_compile_link_options(yaml-cpp NO_EXCEPTIONS NO_RTTI)
  list(APPEND SOURCES tkit/serialization/yaml/codec.cpp)
endif()

# Create the library
add_library(toolkit STATIC ${SOURCES})
target_compile_definitions(toolkit PUBLIC TKIT_VERSION=\"v0.6.0\")

if(TOOLKIT_ENABLE_YAML_SERIALIZATION)
  target_link_libraries(toolkit PUBLIC yaml-cpp)
endif()

if(TOOLKIT_ENABLE_MULTIPROCESSING)
  find_package(PkgConfig QUIET)

  if(PkgConfig_FOUND)
    pkg_check_modules(HWLOC QUIET IMPORTED_TARGET hwloc)
  endif()

  if(HWLOC_FOUND)

    message(STATUS "TOOLKIT - HWLOC found")
    target_link_libraries(toolkit PRIVATE PkgConfig::HWLOC)
    target_compile_definitions(toolkit PRIVATE TKIT_HWLOC_INSTALLED)

  else()
    message(STATUS "TOOLKIT - HWLOC not found")
  endif()
endif()

find_package(fmt)

if(NOT TARGET fmt::fmt)
  message(STATUS "TOOLKIT - FMT not found. Fetching source...")
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.1.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  FetchContent_MakeAvailable(fmt)
  tkit_configure_compile_link_options(fmt NO_EXCEPTIONS NO_RTTI)
  target_compile_definitions(fmt PUBLIC FMT_EXCEPTIONS=0)
  # add no rtti here
else()
  message(STATUS "TOOLKIT - FMT found")
endif()

target_link_libraries(toolkit PUBLIC fmt::fmt)

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

  set(NO_STATISTICS
      OFF
      CACHE BOOL "Super useful doc" FORCE)

  find_package(Tracy QUIET)

  if(NOT TARGET Tracy::TracyClient)

    message(STATUS "TOOLKIT - Tracy not found. Fetching source...")
    FetchContent_Declare(
      tracy
      GIT_REPOSITORY https://github.com/wolfpld/tracy.git
      GIT_TAG v0.12.2
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE)

    FetchContent_MakeAvailable(tracy)
    if(MSVC)
      target_compile_options(TracyClient PRIVATE /wd4996)
    else()
      target_compile_options(TracyClient PRIVATE -Wno-deprecated-declarations)
      target_compile_options(TracyClient
                             PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
    endif()
    if(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION)
      target_compile_definitions(TracyClient PUBLIC TRACY_VK_USE_SYMBOL_TABLE)
      target_compile_definitions(toolkit
                                 PUBLIC TKIT_ENABLE_VULKAN_INSTRUMENTATION)
    endif()
  else()
    message(STATUS "TOOLKIT - Tracy found")
    target_link_libraries(toolkit INTERFACE Tracy::TracyClient)
    target_compile_definitions(toolkit PUBLIC TRACY_ENABLE)
    target_compile_definitions(
      Tracy::TracyClient INTERFACE TRACY_SAMPLING_HZ=${TOOLKIT_SAMPLING_RATE})

    if(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION)
      target_compile_definitions(Tracy::TracyClient
                                 INTERFACE TRACY_VK_USE_SYMBOL_TABLE)

      target_compile_definitions(toolkit
                                 PUBLIC TKIT_ENABLE_VULKAN_INSTRUMENTATION)
    endif()
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

if(TOOLKIT_ENABLE_DEBUG_LOGS)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_DEBUG_LOGS)
endif()

if(TOOLKIT_ENABLE_INFO_LOGS)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_INFO_LOGS)
endif()

if(TOOLKIT_ENABLE_WARNING_LOGS)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_WARNING_LOGS)
endif()

if(TOOLKIT_ENABLE_ERROR_LOGS)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_ERROR_LOGS)
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
