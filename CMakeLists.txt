cmake_minimum_required(VERSION 3.23...4.1.1)
project(toolkit-root)

set(NAME toolkit-root)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_BUILD_TYPE
    "Debug"
    CACHE STRING "")
set(TOOLKIT_BUILD_TESTS
    OFF
    CACHE BOOL "")
set(TOOLKIT_BUILD_PERFORMANCE
    OFF
    CACHE BOOL "")
# NONE, ADDRESS, LEAK, THREAD, UNDEFINED
set(TOOLKIT_ENABLE_SYMBOLS
    OFF
    CACHE BOOL "")
# NONE, FORTIFY, LOW, MEDIUM, HIGH
set(TOOLKIT_OPTIMIZATIONS
    "NONE"
    CACHE STRING "")
set(TOOLKIT_ENABLE_LTO
    OFF
    CACHE BOOL "")
set(TOOLKIT_SANITIZERS
    "NONE"
    CACHE BOOL "")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS
    ""
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS
    ""
    CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE
    ""
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
    ""
    CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE
    ""
    CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
    ""
    CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_DEBUG
    ""
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
    ""
    CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG
    ""
    CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG
    ""
    CACHE STRING "" FORCE)

set(TOOLKIT_ENABLE_DEBUG_LOGS
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_INFO_LOGS
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_WARNING_LOGS
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_ERROR_LOGS
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_ASSERTS
    OFF
    CACHE BOOL "")

set(TOOLKIT_ENABLE_MIMALLOC
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_ARENA_ALLOCATOR
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_BLOCK_ALLOCATOR
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_STACK_ALLOCATOR
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_TIER_ALLOCATOR
    OFF
    CACHE BOOL "")

set(TOOLKIT_ENABLE_MULTIPROCESSING
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_PROFILING
    OFF
    CACHE BOOL "")

# NONE or whatever architecture. native will use AVX2 on windows
set(TOOLKIT_ARCH
    "NONE"
    CACHE STRING "")

set(TOOLKIT_ENABLE_REFLECTION
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_YAML_SERIALIZATION
    OFF
    CACHE BOOL "")

set(TOOLKIT_ENABLE_INSTRUMENTATION
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_SAMPLING
    OFF
    CACHE BOOL "")
set(TOOLKIT_ENABLE_CALLSTACK
    OFF
    CACHE BOOL "")
set(TOOLKIT_SAMPLING_RATE
    "10000"
    CACHE STRING "")

if(TOOLKIT_ENABLE_VULKAN_INSTRUMENTATION AND NOT TOOLKIT_ENABLE_INSTRUMENTATION)
  message(
    FATAL_ERROR
      "TOOLKIT - General purpose instrumentation is required for vulkan instrumentation to work. Disable the latter or enable the former to fix this error"
  )
endif()

set(TOOLKIT_ROOT_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE STRING "" FORCE)
set(TOOLKIT_PYTHON_EXECUTABLE
    python
    CACHE STRING "" FORCE)

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
        "TOOLKIT - Reflection feature is not enabled. Code generation for it will be skipped."
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
        "TOOLKIT - Yaml serialization feature is not enabled. Code generation for it will be skipped."
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

function(remove_duplicates input output)
  set(result "")
  foreach(item IN LISTS ${input})
    if(NOT "${result}" MATCHES "(^|;)${item}(;|$)")
      list(APPEND result "${item}")
    endif()
  endforeach()
  set(${output}
      "${result}"
      PARENT_SCOPE)
endfunction()

function(tkit_remove_compile_options target)
  get_target_property(_opts ${target} COMPILE_OPTIONS)
  if(NOT _opts)
    return()
  endif()

  set(_filtered_opts "")
  foreach(opt IN LISTS _opts)
    set(_remove FALSE)
    foreach(arg IN LISTS ARGN)
      if(opt STREQUAL "${arg}")
        set(_remove TRUE)
      elseif(opt MATCHES "^\\$<.*:${arg}>$")
        set(_remove TRUE)
      endif()
    endforeach()

    if(NOT _remove)
      list(APPEND _filtered_opts "${opt}")
    endif()
  endforeach()

  set_target_properties(${target} PROPERTIES COMPILE_OPTIONS
                                             "${_filtered_opts}")
endfunction()

function(tkit_configure_compile_link_options target)
  set(options FORBID_SANITIZERS NO_EXCEPTIONS NO_RTTI)
  set(multi_value_args COMPILER_FLAGS LINKER_FLAGS)
  set(one_value_args VISIBILITY)

  cmake_parse_arguments(PARSE_ARGV 1 ARG "${options}" "${one_value_args}"
                        "${multi_value_args}")

  if(NOT DEFINED ARG_FORBID_SANITIZERS)
    set(ARG_FORBID_SANITIZERS FALSE)
  endif()
  if(NOT DEFINED ARG_NO_EXCEPTIONS)
    set(ARG_NO_EXCEPTIONS FALSE)
  endif()
  if(NOT DEFINED ARG_NO_RTTI)
    set(ARG_NO_RTTI FALSE)
  endif()
  if(NOT DEFINED ARG_COMPILER_FLAGS)
    set(ARG_COMPILER_FLAGS "")
  endif()
  if(NOT DEFINED ARG_LINKER_FLAGS)
    set(ARG_LINKER_FLAGS "")
  endif()
  if(NOT DEFINED ARG_VISIBILITY)
    set(ARG_VISIBILITY "PRIVATE")
  endif()

  message(STATUS "TOOLKIT - Configuring ${target}...")
  if(ARG_FORBID_SANITIZERS)
    message(STATUS "TOOLKIT -  Sanitizers: Forbidden")
  else()
    message(STATUS "TOOLKIT -  Sanitizers: ${TOOLKIT_SANITIZERS}")
  endif()
  if(ARG_NO_EXCEPTIONS)
    message(STATUS "TOOLKIT -  Exceptions: Disabled")
  else()
    message(STATUS "TOOLKIT -  Exceptions: Enabled")
  endif()
  if(ARG_NO_RTTI)
    message(STATUS "TOOLKIT -  RTTI: Disabled")
  else()
    message(STATUS "TOOLKIT -  RTTI: Enabled")
  endif()
  message(STATUS "TOOLKIT -  Visibility: ${ARG_VISIBILITY}")
  message(STATUS "TOOLKIT -  Provided compiler flags: ${ARG_COMPILER_FLAGS}")
  message(STATUS "TOOLKIT -  Provided linker flags: ${ARG_LINKER_FLAGS}")

  set(compiler_flags "")
  set(linker_flags "")
  if(MSVC)
    list(APPEND compiler_flags /MP)
    list(APPEND linker_flags /INCREMENTAL:NO)
  endif()

  if(TOOLKIT_OPTIMIZATIONS STREQUAL "NONE")
    if(MSVC)
      list(APPEND compiler_flags /Od)
    else()
      list(APPEND compiler_flags -O0)
    endif()
  elseif(TOOLKIT_OPTIMIZATIONS STREQUAL "FORTIFY")
    if(MSVC)
      list(APPEND compiler_flags /Od)
    else()
      list(APPEND compiler_flags -Og)
    endif()
  elseif(TOOLKIT_OPTIMIZATIONS STREQUAL "LOW")
    if(MSVC)
      list(APPEND compiler_flags /O1)
    else()
      list(APPEND compiler_flags -O1)
    endif()
  elseif(TOOLKIT_OPTIMIZATIONS STREQUAL "MEDIUM")
    if(MSVC)
      list(APPEND compiler_flags /O2)
    else()
      list(APPEND compiler_flags -O2)
    endif()
  elseif(TOOLKIT_OPTIMIZATIONS STREQUAL "HIGH")
    if(MSVC)
      list(APPEND compiler_flags /Ox)
    else()
      list(APPEND compiler_flags -O3)
    endif()
  endif()

  if(TOOLKIT_ENABLE_SYMBOLS)
    if(MSVC)
      list(APPEND compiler_flags /ZI /Oy-)
      list(APPEND linker_flags /DEBUG)
    else()
      list(APPEND compiler_flags -g -fno-omit-frame-pointer)
    endif()
  elseif(MSVC)
    list(APPEND compiler_flags /Oy)
  endif()

  if(TOOLKIT_ENABLE_LTO)
    if(MSVC)
      list(APPEND compiler_flags /GL)
      list(APPEND linker_flags /LTCG)
    else()
      list(APPEND compiler_flags -flto)
      list(APPEND linker_flags -flto)
    endif()
  endif()

  if(NOT TOOLKIT_ARCH STREQUAL "NONE")
    string(TOLOWER ${TOOLKIT_ARCH} arch)
    string(TOUPPER ${TOOLKIT_ARCH} ARCH)
    if(MSVC)
      if(TOOLKIT_ARCH STREQUAL "NATIVE")
        list(APPEND compiler_flags /arch:AVX2)
      else()
        list(APPEND compiler_flags /arch:${ARCH})
      endif()
    else()
      list(APPEND compiler_flags -march=${arch})
    endif()
  endif()

  if(NOT TOOLKIT_SANITIZERS STREQUAL "NONE" AND NOT ARG_FORBID_SANITIZERS)
    string(TOLOWER ${TOOLKIT_SANITIZERS} sanitizers)
    if(MSVC)
      list(APPEND compiler_flags /fsanitize=${sanitizers} /ZI)
    else()
      list(APPEND compiler_flags -fsanitize=${sanitizers} -g
           -fno-omit-frame-pointer)
      list(APPEND linker_flags -fsanitize=${sanitizers} -g
           -fno-omit-frame-pointer)
    endif()

  endif()

  if(ARG_NO_EXCEPTIONS)
    if(MSVC)
      tkit_remove_compile_options(${target} /EHs)
      list(APPEND compiler_flags /EHs- /D_HAS_EXCEPTIONS=0)
    else()
      list(APPEND compiler_flags -fno-exceptions)
    endif()
  endif()

  if(ARG_NO_RTTI)
    if(MSVC)
      tkit_remove_compile_options(${target} /GR)
      list(APPEND compiler_flags /GR-)
    else()
      list(APPEND compiler_flags -fno-rtti)
    endif()
  endif()

  if(TOOLKIT_ENABLE_SAMPLING)
    if(MSVC)
      list(APPEND compiler_flags /ZI)
    else()
      list(APPEND compiler_flags -g -fno-omit-frame-pointer)
    endif()

  endif()

  remove_duplicates(compiler_flags ucompiler_flags)
  remove_duplicates(linker_flags ulinker_flags)
  message(
    STATUS
      "TOOLKIT -  Final compiler flags: ${ucompiler_flags} ${ARG_COMPILER_FLAGS}"
  )
  message(
    STATUS "TOOLKIT -  Final linker flags: ${ulinker_flags} ${ARG_LINKER_FLAGS}"
  )
  target_compile_options(${target} ${ARG_VISIBILITY} ${ucompiler_flags}
                         ${ARG_COMPILER_FLAGS})
  target_link_options(${target} ${ARG_VISIBILITY} ${ulinker_flags}
                      ${ARG_LINKER_FLAGS})

endfunction()

function(tkit_default_configure target)
  if(MSVC)
    tkit_configure_compile_link_options(${target} COMPILER_FLAGS /W4 /WX
                                        NO_EXCEPTIONS NO_RTTI)
  elseif((UNIX AND APPLE) OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    tkit_configure_compile_link_options(
      ${target}
      COMPILER_FLAGS
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      NO_EXCEPTIONS
      NO_RTTI)
  else()
    tkit_configure_compile_link_options(
      ${target}
      COMPILER_FLAGS
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wno-changes-meaning
      NO_EXCEPTIONS
      NO_RTTI)
  endif()
endfunction()

add_subdirectory(toolkit)

if(TOOLKIT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(TOOLKIT_BUILD_PERFORMANCE)
  add_subdirectory(performance)
endif()
