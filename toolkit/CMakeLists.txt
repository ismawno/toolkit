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

if(TOOLKIT_ENABLE_MULTIPROCESSING)
  list(APPEND SOURCES tkit/multiprocessing/task.cpp
       tkit/multiprocessing/task_manager.cpp
       tkit/multiprocessing/thread_pool.cpp)
endif()

if(TOOLKIT_ENABLE_PROFILING)
  list(APPEND SOURCES tkit/profiling/clock.cpp tkit/profiling/timespan.cpp)
endif()

include(FetchContent)
if(TOOLKIT_ENABLE_YAML_SERIALIZATION)

  set(YAML_CPP_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS
      OFF
      CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB
      OFF
      CACHE BOOL "" FORCE)
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0)

  FetchContent_MakeAvailable(yaml-cpp)

  tkit_configure_compile_link_options(yaml-cpp FORBID_SANITIZERS)
  list(APPEND SOURCES tkit/serialization/yaml/codec.cpp)
endif()

# Create the library
add_library(toolkit STATIC ${SOURCES})

if(TOOLKIT_ENABLE_YAML_SERIALIZATION)
  target_link_libraries(toolkit PUBLIC yaml-cpp)
endif()

if(UNIX AND NOT APPLE)
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.0.2)
  FetchContent_MakeAvailable(fmt)

  tkit_configure_compile_link_options(fmt FORBID_SANITIZERS)
  target_link_libraries(toolkit PUBLIC fmt::fmt)
endif()

function(configure_tracy target)
  if(MSVC)
    tkit_configure_compile_link_options(${target} FLAGS /wd4996 CONFIG Dist
                                        FORBID_SANITIZERS)
  elseif(APPLE)
    tkit_configure_compile_link_options(
      ${target}
      FLAGS
      -Wno-format
      -Wno-deprecated-declarations
      -Wno-deprecated-anon-enum-enum-conversion
      CONFIG
      Dist
      FORBID_SANITIZERS)
  else()
    tkit_configure_compile_link_options(
      ${target}
      FLAGS
      -Wno-format
      -Wno-deprecated-declarations
      -Wno-deprecated-enum-enum-conversion
      CONFIG
      Dist
      FORBID_SANITIZERS)
  endif()
endfunction()

if(TOOLKIT_ENABLE_INSTRUMENTATION)
  FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.12.2)
  FetchContent_MakeAvailable(tracy)
  target_link_libraries(toolkit PUBLIC TracyClient)
  target_compile_definitions(toolkit PUBLIC TRACY_ENABLE)
  target_compile_definitions(toolkit PUBLIC TKIT_ENABLE_INSTRUMENTATION)

  # Because of this check, the profiler binaries will always persist and will
  # keep the first cmake options that were set. Recompile with other options by
  # removing the profiler directory inside your build directory

  set(NO_STATISTICS
      OFF
      CACHE BOOL "I hate cmake" FORCE)
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/profiler/gui AND TOOLKIT_BUILD_PROFILER
                                                     STREQUAL "gui")
    add_subdirectory(${tracy_SOURCE_DIR}/profiler
                     ${CMAKE_BINARY_DIR}/profiler/gui)
    configure_tracy(tracy-profiler)
    configure_tracy(TracyServer)
  endif()
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/profiler/cli AND TOOLKIT_BUILD_PROFILER
                                                     STREQUAL "cli")
    add_subdirectory(${tracy_SOURCE_DIR}/capture
                     ${CMAKE_BINARY_DIR}/profiler/cli)
    configure_tracy(tracy-capture)
    configure_tracy(TracyServer)
  endif()
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/profiler/csv AND TOOLKIT_BUILD_PROFILER
                                                     STREQUAL "csv")
    add_subdirectory(${tracy_SOURCE_DIR}/csvexport
                     ${CMAKE_BINARY_DIR}/profiler/csv)
    configure_tracy(tracy-csvexport)
    configure_tracy(TracyServer)
  endif()

  if(MSVC)
    target_compile_options(TracyClient PRIVATE /wd4996)
  else()
    target_compile_options(TracyClient PRIVATE -Wno-deprecated-declarations)
    target_compile_options(TracyClient
                           PRIVATE -Wno-deprecated-anon-enum-enum-conversion)
  endif()

  if(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION)
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
