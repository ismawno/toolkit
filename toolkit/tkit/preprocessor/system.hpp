#pragma once

#ifdef _WIN32
#    define TKIT_OS_WINDOWS
#    undef min
#    undef max
#    undef TRANSPARENT
#    define NOMINMAX
#    if defined(_WIN64)
#        define TKIT_OS_WINDOWS_64
#    else
#        define TKIT_OS_WINDOWS_32
#    endif
#elif defined(__linux__)
#    define TKIT_OS_LINUX
#elif defined(__APPLE__)
#    define TKIT_OS_APPLE
#elif defined(__ANDROID__)
#    define TKIT_OS_ANDROID
#endif

#ifdef __MACH__
#    define TKIT_MACOS
#endif

#if defined(_M_IX86) || defined(__i386__)
#    define TKIT_32_BIT_ARCH
#elif defined(_M_X64) || defined(__x86_64__)
#    define TKIT_64_BIT_ARCH
#elif defined(__arm__) || defined(_M_ARM)
#    define TKIT_ARM
#elif defined(__aarch64__) || defined(_M_ARM64)
#    define TKIT_ARM64
#endif

#ifdef __clang__
#    define TKIT_COMPILER_CLANG
#    define TKIT_COMPILER_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#    define TKIT_COMPILER_GCC
#    define TKIT_COMPILER_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#    define TKIT_COMPILER_MSVC
#    define TKIT_COMPILER_MSVC_VERSION _MSC_VER
#    define TKIT_COMPILER_MSVC_VERSION_FULL _MSC_FULL_VER
#endif

// #ifdef TKIT_OS_WINDOWS
// #    ifdef TKIT_SHARED_LIBRARY
// #        ifdef TKIT_EXPORT
// #            define TKIT_API __declspec(dllexport)
// #        else
// #            define TKIT_API __declspec(dllimport)
// #        endif
// #    else
// #        define TKIT_API
// #    endif
// #elif defined(TKIT_OS_LINUX) || defined(TKIT_OS_APPLE)
// #    if defined(TKIT_SHARED_LIBRARY) && defined(TKIT_EXPORT)
// #        define TKIT_API __attribute__((visibility("default")))
// #    else
// #        define TKIT_API
// #    endif
// #endif

#ifdef TKIT_COMPILER_CLANG
#    define TKIT_PRAGMA(arg) _Pragma(#arg)
#    define TKIT_COMPILER_WARNING_IGNORE_PUSH() TKIT_PRAGMA(clang diagnostic push)
#    define TKIT_COMPILER_WARNING_IGNORE_POP() TKIT_PRAGMA(clang diagnostic pop)
#    define TKIT_CLANG_WARNING_IGNORE(warning) TKIT_PRAGMA(clang diagnostic ignored warning)
#else
#    define TKIT_CLANG_WARNING_IGNORE(warning)
#endif
#ifdef TKIT_COMPILER_GCC
#    define TKIT_PRAGMA(x) _Pragma(#x)
#    define TKIT_COMPILER_WARNING_IGNORE_PUSH() TKIT_PRAGMA(GCC diagnostic push)
#    define TKIT_COMPILER_WARNING_IGNORE_POP() TKIT_PRAGMA(GCC diagnostic pop)
#    define TKIT_GCC_WARNING_IGNORE(warning) TKIT_PRAGMA(GCC diagnostic ignored warning)
#else
#    define TKIT_GCC_WARNING_IGNORE(warning)
#endif
#ifdef TKIT_COMPILER_MSVC
#    define TKIT_PRAGMA(x) __pragma(x)
#    define TKIT_COMPILER_WARNING_IGNORE_PUSH() TKIT_PRAGMA(warning(push))
#    define TKIT_COMPILER_WARNING_IGNORE_POP() TKIT_PRAGMA(warning(pop))
#    define TKIT_MSVC_WARNING_IGNORE(warning) TKIT_PRAGMA(warning(disable : warning))
#else
#    define TKIT_MSVC_WARNING_IGNORE(warning)
#endif

#ifdef TKIT_COMPILER_MSVC
#    define TKIT_NO_RETURN __declspec(noreturn)
#else
#    define TKIT_NO_RETURN __attribute__((noreturn))
#endif

#define TKIT_PUSH_MACRO(macro) TKIT_PRAGMA(push_macro(#macro))
#define TKIT_POP_MACRO(macro) TKIT_PRAGMA(pop_macro(#macro))

#if defined(TKIT_OS_WINDOWS) || defined(TKIT_OS_APPLE)
#    define TKIT_CONSTEVAL consteval
#else
#    define TKIT_CONSTEVAL constexpr
#endif

#if defined(TKIT_COMPILER_GCC) || defined(TKIT_COMPILER_CLANG)
#    define TKIT_UNREACHABLE() __builtin_unreachable()
#    define TKIT_FORCE_INLINE inline __attribute__((always_inline))
#    define TKIT_NO_INLINE __attribute__((noinline))
#elif defined(TKIT_COMPILER_MSVC)
#    define TKIT_UNREACHABLE() __assume(false)
#    define TKIT_FORCE_INLINE __forceinline
#    define TKIT_NO_INLINE __declspec(noinline)
#else
#    define TKIT_UNREACHABLE()
#    define TKIT_FORCE_INLINE
#    define TKIT_NO_INLINE
#endif

#ifndef TKIT_CACHE_LINE_SIZE
#    define TKIT_CACHE_LINE_SIZE 64
#endif

// Size is in bytes
// x86
#ifdef __AVX512F__
#    define TKIT_SIMD_AVX512F
#    define TKIT_SIMD_AVX512F_SIZE 64
#endif
#ifdef __AVX2__
#    define TKIT_SIMD_AVX2
#endif
#ifdef __AVX__
#    define TKIT_SIMD_AVX_SIZE 32
#    define TKIT_SIMD_AVX
#endif

#ifdef __SSE4_2__
#    define TKIT_SIMD_SSE4_2
#endif
#ifdef __SSE4_1__
#    define TKIT_SIMD_SSE4_1
#endif
#ifdef __SSSE3__
#    define TKIT_SIMD_SSSE3
#endif
#ifdef __SSE3__
#    define TKIT_SIMD_SSE3
#endif
#ifdef __SSE2__
#    define TKIT_SIMD_SSE2
#    define TKIT_SIMD_SSE_SIZE 16
#endif

// ARM
#ifdef __ARM_FEATURE_SVE2
#    define TKIT_SIMD_SVE2
#endif
#ifdef __ARM_FEATURE_SVE
#    define TKIT_SIMD_SVE
#endif

#if defined(TKIT_SIMD_SVE2) || defined(TKIT_SIMD_SVE)
#    ifdef __ARM_FEATURE_SVE_BITS
#        define TKIT_SIMD_SVE_SIZE (__ARM_FEATURE_SVE_BITS >> 3)
#    else
#        define TKIT_SIMD_SVE_SIZE 16 // a bit made up
#    endif
#    ifndef TKIT_SIMD_SIZE
#        define TKIT_SIMD_SIZE TKIT_SIMD_SVE_SIZE
#    endif
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#    define TKIT_SIMD_NEON
#    define TKIT_SIMD_NEON_SIZE 16
#endif

#if defined(__aarch64__) || defined(__arm64ec__)
#    define TKIT_AARCH64
#endif

// PowerPC
#ifdef __VSX__
#    define TKIT_SIMD_VSX
#    define TKIT_SIMD_VSX_SIZE 16
#    ifndef TKIT_SIMD_SIZE
#        define TKIT_SIMD_SIZE TKIT_SIMD_VSX_SIZE
#    endif
#endif
#ifdef __ALTIVEC__
#    define TKIT_SIMD_ALTIVEC
#    define TKIT_SIMD_ALTIVEC_SIZE 16
#    ifndef TKIT_SIMD_SIZE
#        define TKIT_SIMD_SIZE TKIT_SIMD_ALTIVEC_SIZE
#    endif
#endif

// RISC-V
#ifdef __riscv_vector
#    define TKIT_SIMD_RISCV_V
#    define TKIT_SIMD_RISCV_V_SIZE 16 // a bit made up
// #ifndef TKIT_SIMD_SIZE
// #define TKIT_SIMD_SIZE TKIT_SIMD_RISCV_V_SIZE
// #endif
#endif

// WebAssembly
#ifdef __wasm_simd128__
#    define TKIT_SIMD_WASM_SIMD128
#    define TKIT_SIMD_WASM_SIMD128_SIZE 16
#    ifndef TKIT_SIMD_SIZE
#        define TKIT_SIMD_SIZE TKIT_SIMD_WASM_SIMD128_SIZE
#    endif
#endif

// MIPS
#ifdef __mips_msa
#    define TKIT_SIMD_MIPS_MSA
#    define TKIT_SIMD_MIPS_MSA_SIZE 16
#    ifndef TKIT_SIMD_SIZE
#        define TKIT_SIMD_SIZE TKIT_SIMD_MIPS_MSA_SIZE
#    endif
#endif

#ifdef __BMI2__
#    define TKIT_BMI2
#endif

#ifdef __BMI__
#    define TKIT_BMI
#endif
