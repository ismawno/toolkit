cmake_minimum_required(VERSION 3.16)
project(toolkit-root)

set(NAME toolkit-root)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist")

function(define_option varname default)
  if(NOT DEFINED ${varname})
    set(${varname}
        ${default}
        PARENT_SCOPE)
  endif()
endfunction()

define_option(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_RELEASE
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
    ""
    CACHE STRING "I hate cmake" FORCE)

set(CMAKE_C_FLAGS_DEBUG
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG
    ""
    CACHE STRING "I hate cmake" FORCE)

set(CMAKE_C_FLAGS_DIST
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_CXX_FLAGS_DIST
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DIST
    ""
    CACHE STRING "I hate cmake" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DIST
    ""
    CACHE STRING "I hate cmake" FORCE)

define_option(TOOLKIT_ENABLE_INFO_LOGS OFF)
define_option(TOOLKIT_ENABLE_WARNING_LOGS OFF)
define_option(TOOLKIT_ENABLE_ASSERTS OFF)

define_option(TOOLKIT_ENABLE_ARENA_ALLOCATOR OFF)
define_option(TOOLKIT_ENABLE_BLOCK_ALLOCATOR OFF)
define_option(TOOLKIT_ENABLE_STACK_ALLOCATOR OFF)

define_option(TOOLKIT_ENABLE_MULTIPROCESSING OFF)
define_option(TOOLKIT_ENABLE_PROFILING OFF)

define_option(TOOLKIT_ENABLE_INSTRUMENTATION OFF)
define_option(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION OFF)

define_option(TOOLKIT_BUILD_TESTS OFF)
define_option(TOOLKIT_BUILD_PERFORMANCE OFF)
# none, gui, cli or csv
define_option(TOOLKIT_BUILD_PROFILER "none")
# none, address, leak, thread, undefined
define_option(TOOLKIT_SANITIZERS "none")
define_option(TOOLKIT_ENABLE_REFLECTION OFF)
define_option(TOOLKIT_ENABLE_YAML_SERIALIZATION OFF)

if(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION AND NOT TOOLKIT_ENABLE_INSTRUMENTATION)
  message(
    WARNING
      "TOOLKIT: Vulkan instrumentation is enabled but general instrumentation is not. This option will have no effect"
  )
endif()

if(TOOLKIT_BUILD_PERFORMANCE AND (NOT CMAKE_BUILD_TYPE STREQUAL "Dist"))
  message(
    WARNING
      "TOOLKIT: Building benchmarks in a non-distribution configuration! Results may not be representative"
  )
endif()

if(TOOLKIT_BUILD_PERFORMANCE AND (TOOLKIT_ENABLE_WARNING_LOGS
                                  OR TOOLKIT_ENABLE_ASSERTS))
  message(
    WARNING
      "TOOLKIT: Building benchmarks with asserts/logs enabled! Results may not be representative"
  )
endif()

if(NOT TOOLKIT_ENABLE_INSTRUMENTATION AND NOT TOOLKIT_BUILD_PROFILER STREQUAL
                                          "none")
  message(
    WARNING "TOOLKIT: To build the profiler, instrumentation must be enabled")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(NOT TOOLKIT_ENABLE_INFO_LOGS)
    message(
      WARNING
        "TOOLKIT: Info logs are disabled in a debug build. This is not recommended"
    )
  endif()
  if(NOT TOOLKIT_ENABLE_WARNING_LOGS)
    message(
      WARNING
        "TOOLKIT: Warning logs are disabled in a debug build. This is not recommended"
    )
  endif()
  if(NOT TOOLKIT_ENABLE_ASSERTS)
    message(
      WARNING
        "TOOLKIT: Asserts are disabled in a debug build. This is not recommended"
    )
  endif()
endif()

message(STATUS "TOOLKIT: Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "TOOLKIT: Build tests: ${TOOLKIT_BUILD_TESTS}")
message(STATUS "TOOLKIT: Build performance tests: ${TOOLKIT_BUILD_PERFORMANCE}")
message(STATUS "TOOLKIT: Enable info logs: ${TOOLKIT_ENABLE_INFO_LOGS}")
message(STATUS "TOOLKIT: Enable warning logs: ${TOOLKIT_ENABLE_WARNING_LOGS}")
message(STATUS "TOOLKIT: Enable asserts: ${TOOLKIT_ENABLE_ASSERTS}")
message(
  STATUS "TOOLKIT: Enable arena allocator: ${TOOLKIT_ENABLE_ARENA_ALLOCATOR}")
message(
  STATUS "TOOLKIT: Enable block allocator: ${TOOLKIT_ENABLE_BLOCK_ALLOCATOR}")
message(
  STATUS "TOOLKIT: Enable stack allocator: ${TOOLKIT_ENABLE_STACK_ALLOCATOR}")
message(
  STATUS "TOOLKIT: Enable multiprocessing: ${TOOLKIT_ENABLE_MULTIPROCESSING}")
message(
  STATUS
    "TOOLKIT: Enable profiling: ${TOOLKIT_ENABLE_PROFILING} (This only enables features located under the 'profiling' directory. To enable the instrumentation profiling macros, set TOOLKIT_ENABLE_INSTRUMENTATION to ON)"
)
message(
  STATUS
    "TOOLKIT: Enable yaml serialization: ${TOOLKIT_ENABLE_YAML_SERIALIZATION}")
message(
  STATUS
    "TOOLKIT: Sanitizer: ${TOOLKIT_SANITIZERS} (can be none, address, leak, thread, undefined)"
)
message(STATUS "TOOLKIT: Instrumentation: ${TOOLKIT_ENABLE_INSTRUMENTATION}")
message(
  STATUS
    "TOOLKIT: Vulkan instrumentation: ${TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION}")
message(
  STATUS
    "TOOLKIT: Build profiler: ${TOOLKIT_BUILD_PROFILER} (can be none, gui, cli or csv)"
)

message(
  STATUS "TOOLKIT: The toolkit root path is set to ${CMAKE_CURRENT_SOURCE_DIR}")
set(TOOLKIT_ROOT_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE STRING "I hate cmake" FORCE)
set(TOOLKIT_PYTHON_EXECUTABLE
    python
    CACHE STRING "I hate cmake" FORCE)

function(
  tkit_register_code_generation_target
  name
  target
  directory
  src
  pypath
  args)

  string(REPLACE / _ src_underscore ${src})
  string(REPLACE .cpp .hpp src_hpp ${src})
  set(SRC_NAME "${name}_${src_underscore}")
  set(OUTPUT_PATH "${TOOLKIT_ROOT_PATH}/${directory}/${src_hpp}")

  add_custom_command(
    OUTPUT ${OUTPUT_PATH}
    COMMAND ${TOOLKIT_PYTHON_EXECUTABLE} ${TOOLKIT_ROOT_PATH}/${pypath} -i
            ${CMAKE_CURRENT_SOURCE_DIR}/${src} -o ${OUTPUT_PATH} -v ${args}
    DEPENDS ${src}
    VERBATIM)

  add_custom_target(${SRC_NAME} DEPENDS ${OUTPUT_PATH})
  add_dependencies(${target} ${SRC_NAME})
endfunction()

function(tkit_register_for_reflection target)
  set(multi_value_args SOURCES)

  if(NOT TOOLKIT_ENABLE_REFLECTION)
    message(
      STATUS
        "TOOLKIT: Reflection feature is not enabled. Code generation for it will be skipped."
    )
    return()
  endif()

  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "${multi_value_args}")
  foreach(src ${ARG_SOURCES})
    tkit_register_code_generation_target(
      reflection ${target} toolkit/tkit/reflection ${src}
      codegen/cpp/reflect.py "")
  endforeach()
endfunction()

function(tkit_register_for_yaml_serialization target)
  set(multi_value_args SOURCES)

  if(NOT TOOLKIT_ENABLE_YAML_SERIALIZATION)
    message(
      STATUS
        "TOOLKIT: Yaml serialization feature is not enabled. Code generation for it will be skipped."
    )
    return()
  endif()

  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "${multi_value_args}")
  foreach(src ${ARG_SOURCES})
    tkit_register_code_generation_target(
      yaml_serialization ${target} toolkit/tkit/serialization/yaml ${src}
      codegen/cpp/serialize.py "-b yaml")
  endforeach()
endfunction()

function(tkit_configure_compile_link_options target)
  set(options FORBID_SANITIZERS)
  set(multi_value_args FLAGS)
  set(one_value_args CONFIG VISIBILITY)

  cmake_parse_arguments(PARSE_ARGV 1 ARG "${options}" "${one_value_args}"
                        "${multi_value_args}")
  if(NOT DEFINED ARG_CONFIG)
    set(ARG_CONFIG ${CMAKE_BUILD_TYPE})
  endif()
  if(NOT DEFINED ARG_FORBID_SANITIZERS)
    set(ARG_FORBID_SANITIZERS FALSE)
  endif()
  if(NOT DEFINED ARG_FLAGS)
    set(ARG_FLAGS "")
  endif()
  if(NOT DEFINED ARG_VISIBILITY)
    set(ARG_VISIBILITY "PRIVATE")
  endif()

  message(STATUS "TOOLKIT: Configuring ${target}...")
  message(STATUS "TOOLKIT:     Configuration: ${ARG_CONFIG}")
  if(ARG_FORBID_SANITIZERS)
    message(STATUS "TOOLKIT:     Sanitizers: Forbidden")
  else()
    message(STATUS "TOOLKIT:     Sanitizers: ${TOOLKIT_SANITIZERS}")
  endif()
  message(STATUS "TOOLKIT:     Visibility: ${ARG_VISIBILITY}")
  message(STATUS "TOOLKIT:     Provided flags: ${ARG_FLAGS}")

  if(ARG_CONFIG STREQUAL "Debug")
    if(MSVC)
      target_compile_options(${target} ${ARG_VISIBILITY} /ZI /Od /MP
                             ${ARG_FLAGS})
      target_link_options(${target} ${ARG_VISIBILITY} /DEBUG /INCREMENTAL:NO)
      message(STATUS "TOOLKIT:     Compiler flags: /ZI;/Od;/MP")
      message(STATUS "TOOLKIT:     Linker flags: /DEBUG;/INCREMENTAL:NO")
    else()
      target_compile_options(${target} ${ARG_VISIBILITY} -g -O0 ${ARG_FLAGS})
      message(STATUS "TOOLKIT:     Compiler flags: -g;-O0")
    endif()

  elseif(ARG_CONFIG STREQUAL "Release")
    if(MSVC)
      target_compile_options(${target} ${ARG_VISIBILITY} /ZI /O2 /MP
                             ${ARG_FLAGS})
      target_link_options(${target} ${ARG_VISIBILITY} /DEBUG /INCREMENTAL:NO)
      message(STATUS "TOOLKIT:     Compiler flags: /ZI;/O2;/MP")
      message(STATUS "TOOLKIT:     Linker flags: /DEBUG;/INCREMENTAL:NO")
    else()
      target_compile_options(${target} ${ARG_VISIBILITY} -g -O2 ${ARG_FLAGS})
      message(STATUS "TOOLKIT:     Compiler flags: -g;-O2")
    endif()

  elseif(ARG_CONFIG STREQUAL "Dist")
    if(MSVC)
      target_compile_options(${target} ${ARG_VISIBILITY} /Ox /GL /MP
                             ${ARG_FLAGS})
      target_link_options(${target} ${ARG_VISIBILITY} /LTCG /INCREMENTAL:NO)
      message(STATUS "TOOLKIT:     Compiler flags: /Ox;/GL;/MP")
      message(STATUS "TOOLKIT:     Linker flags: /LTCG;/INCREMENTAL:NO")
    else()
      target_compile_options(${target} ${ARG_VISIBILITY} -O3 -flto ${ARG_FLAGS})
      target_link_options(${target} ${ARG_VISIBILITY} -flto)
      message(STATUS "TOOLKIT:     Compiler flags: -O3;-flto")
      message(STATUS "TOOLKIT:     Linker flags: -flto")
    endif()
  endif()

  if(NOT DEFINED TOOLKIT_SANITIZERS
     OR TOOLKIT_SANITIZERS STREQUAL "none"
     OR ARG_FORBID_SANITIZERS)
    return()
  endif()

  if(MSVC)
    target_compile_options(${target} ${ARG_VISIBILITY}
                           /fsanitize=${TOOLKIT_SANITIZERS} /FS)
    message(
      STATUS
        "TOOLKIT:     Sanitizer compiler flags: /fsanitize=${TOOLKIT_SANITIZERS};/FS"
    )
  else()
    target_compile_options(
      ${target} ${ARG_VISIBILITY} -fsanitize=${TOOLKIT_SANITIZERS}
      -fno-omit-frame-pointer)
    target_link_options(
      ${target} ${ARG_VISIBILITY} -fsanitize=${TOOLKIT_SANITIZERS}
      -fno-omit-frame-pointer)
    message(
      STATUS
        "TOOLKIT:     Sanitizer compiler flags: -fsanitize=${TOOLKIT_SANITIZERS};-fno-omit-frame-pointer"
    )
    message(
      STATUS
        "TOOLKIT:     Sanitizer linker flags: -fsanitize=${TOOLKIT_SANITIZERS};-fno-omit-frame-pointer"
    )
  endif()
endfunction()

function(tkit_default_configure target)
  if(MSVC)
    tkit_configure_compile_link_options(${target} FLAGS /W4 /WX)
  elseif((UNIX AND APPLE) OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    tkit_configure_compile_link_options(${target} FLAGS -Wall -Wextra
                                        -Wpedantic -Werror)
  else()
    tkit_configure_compile_link_options(
      ${target}
      FLAGS
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wno-changes-meaning)
  endif()
endfunction()

# Add subdirectories
add_subdirectory(toolkit)

if(TOOLKIT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(TOOLKIT_BUILD_PERFORMANCE)
  message(
    WARNING
      "TOOLKIT: By building performance tests, the stack allocator maximum entries will be forced to 10000."
  )
  target_compile_definitions(toolkit
                             PUBLIC TKIT_STACK_ALLOCATOR_MAX_ENTRIES=10000)
  add_subdirectory(performance)
endif()
