#pragma once

#ifdef _WIN32
#    define KIT_OS_WINDOWS
#    if defined(_WIN64)
#        define KIT_OS_WINDOWS_64
#    else
#        define KIT_OS_WINDOWS_32
#    endif
#elif defined(__linux__)
#    define KIT_OS_LINUX
#elif defined(__APPLE__)
#    define KIT_OS_APPLE
#endif

#ifdef __MACH__
#    define KIT_MACOS
#endif

#if defined(_M_IX86) || defined(__i386__)
#    define KIT_32_BIT_ARCH
#elif defined(_M_X64) || defined(__x86_64__)
#    define KIT_64_BIT_ARCH
#elif defined(__arm__) || defined(_M_ARM)
#    define KIT_ARM
#elif defined(__aarch64__) || defined(_M_ARM64)
#    define KIT_ARM64
#endif

#ifdef __clang__
#    define KIT_COMPILER_CLANG
#    define KIT_COMPILER_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#    define KIT_COMPILER_GCC
#    define KIT_COMPILER_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#    define KIT_COMPILER_MSVC
#    define KIT_COMPILER_MSVC_VERSION _MSC_VER
#    define KIT_COMPILER_MSVC_VERSION_FULL _MSC_FULL_VER
#endif

#ifdef KIT_OS_WINDOWS
#    ifdef KIT_SHARED_LIBRARY
#        ifdef KIT_EXPORT
#            define KIT_API __declspec(dllexport)
#        else
#            define KIT_API __declspec(dllimport)
#        endif
#    else
#        define KIT_API
#    endif
#elif defined(KIT_OS_LINUX) || defined(KIT_OS_APPLE)
#    if defined(KIT_SHARED_LIBRARY) && defined(KIT_EXPORT)
#        define KIT_API __attribute__((visibility("default")))
#    else
#        define KIT_API
#    endif
#endif

#ifdef KIT_COMPILER_CLANG
#    define KIT_PRAGMA(x) _Pragma(#x)
#    define KIT_WARNING_IGNORE_PUSH KIT_PRAGMA(clang diagnostic push)
#    define KIT_WARNING_IGNORE_POP KIT_PRAGMA(clang diagnostic pop)
#    define KIT_CLANG_WARNING_IGNORE(wrng) KIT_PRAGMA(clang diagnostic ignored wrng)
#else
#    define KIT_CLANG_WARNING_IGNORE(wrng)
#endif
#ifdef KIT_COMPILER_GCC
#    define KIT_PRAGMA(x) _Pragma(#x)
#    define KIT_WARNING_IGNORE_PUSH KIT_PRAGMA(GCC diagnostic push)
#    define KIT_WARNING_IGNORE_POP KIT_PRAGMA(GCC diagnostic pop)
#    define KIT_GCC_WARNING_IGNORE(wrng) KIT_PRAGMA(GCC diagnostic ignored wrng)
#else
#    define KIT_GCC_WARNING_IGNORE(wrng)
#endif
#ifdef KIT_COMPILER_MSVC
#    define KIT_PRAGMA(x) __pragma(x)
#    define KIT_WARNING_IGNORE_PUSH KIT_PRAGMA(warning(push))
#    define KIT_WARNING_IGNORE_POP KIT_PRAGMA(warning(pop))
#    define KIT_MSVC_WARNING_IGNORE(wrng) KIT_PRAGMA(warning(disable : wrng))
#else
#    define KIT_MSVC_WARNING_IGNORE(wrng)
#endif

#ifdef KIT_ENABLE_THROW
#    define KIT_NOEXCEPT
#else
#    define KIT_NOEXCEPT noexcept
#endif

#ifndef KIT_NAMESPACE_NAME
#    define KIT_NAMESPACE_NAME KIT
#endif

#define KIT_NAMESPACE_BEGIN                                                                                            \
    namespace KIT_NAMESPACE_NAME                                                                                       \
    {
#define KIT_NAMESPACE_END }