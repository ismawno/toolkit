#pragma once

#ifdef _WIN32
#    define TKIT_OS_WINDOWS
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

#ifdef TKIT_OS_WINDOWS
#    ifdef TKIT_SHARED_LIBRARY
#        ifdef TKIT_EXPORT
#            define TKIT_API __declspec(dllexport)
#        else
#            define TKIT_API __declspec(dllimport)
#        endif
#    else
#        define TKIT_API
#    endif
#elif defined(TKIT_OS_LINUX) || defined(TKIT_OS_APPLE)
#    if defined(TKIT_SHARED_LIBRARY) && defined(TKIT_EXPORT)
#        define TKIT_API __attribute__((visibility("default")))
#    else
#        define TKIT_API
#    endif
#endif

#ifdef TKIT_COMPILER_CLANG
#    define TKIT_PRAGMA(p_Arg) _Pragma(#p_Arg)
#    define TKIT_COMPILER_WARNING_IGNORE_PUSH() TKIT_PRAGMA(clang diagnostic push)
#    define TKIT_COMPILER_WARNING_IGNORE_POP() TKIT_PRAGMA(clang diagnostic pop)
#    define TKIT_CLANG_WARNING_IGNORE(p_Warning) TKIT_PRAGMA(clang diagnostic ignored p_Warning)
#else
#    define TKIT_CLANG_WARNING_IGNORE(p_Warning)
#endif
#ifdef TKIT_COMPILER_GCC
#    define TKIT_PRAGMA(x) _Pragma(#x)
#    define TKIT_COMPILER_WARNING_IGNORE_PUSH() TKIT_PRAGMA(GCC diagnostic push)
#    define TKIT_COMPILER_WARNING_IGNORE_POP() TKIT_PRAGMA(GCC diagnostic pop)
#    define TKIT_GCC_WARNING_IGNORE(p_Warning) TKIT_PRAGMA(GCC diagnostic ignored p_Warning)
#else
#    define TKIT_GCC_WARNING_IGNORE(p_Warning)
#endif
#ifdef TKIT_COMPILER_MSVC
#    define TKIT_PRAGMA(x) __pragma(x)
#    define TKIT_COMPILER_WARNING_IGNORE_PUSH() TKIT_PRAGMA(warning(push))
#    define TKIT_COMPILER_WARNING_IGNORE_POP() TKIT_PRAGMA(warning(pop))
#    define TKIT_MSVC_WARNING_IGNORE(p_Warning) TKIT_PRAGMA(warning(disable : p_Warning))
#else
#    define TKIT_MSVC_WARNING_IGNORE(p_Warning)
#endif

#define TKIT_PUSH_MACRO(p_Macro) TKIT_PRAGMA(push_macro(#p_Macro))
#define TKIT_POP_MACRO(p_Macro) TKIT_PRAGMA(pop_macro(#p_Macro))

#if defined(TKIT_OS_WINDOWS) || defined(TKIT_OS_APPLE)
#    define TKIT_CONSTEVAL consteval
#else
#    define TKIT_CONSTEVAL constexpr
#endif

#ifndef TKIT_CACHE_LINE_SIZE
#    define TKIT_CACHE_LINE_SIZE 64
#endif

#define TKIT_UNUSED(x) (void)(x)

#define __TKIT_ARG_SEQ(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, \
                       _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,  \
                       _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59,  \
                       _60, _61, _62, _63, N, ...)                                                                     \
    N
#define __TKIT_NSEQ()                                                                                                  \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,    \
        35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8,  \
        7, 6, 5, 4, 3, 2, 1, 0

#define __TKIT_NARG(...) __TKIT_ARG_SEQ(__VA_ARGS__)

#define TKIT_NARG(...) __TKIT_NARG(__VA_ARGS__, __TKIT_NSEQ())