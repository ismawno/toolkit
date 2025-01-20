#pragma once

#include "tkit/core/api.hpp"
#include "tkit/core/alias.hpp"

#if !defined(TKIT_ENABLE_INFO_LOGS) && !defined(TKIT_ENABLE_WARNING_LOGS) && !defined(TKIT_ENABLE_ASSERTS)
#    define __TKIT_NO_LOGS
#endif

// We dont use enough formatting features to justify the overhead of fmtlib, but linux doesnt have std::format, so I
// am forced to use it in that case
#ifdef TKIT_OS_LINUX
#    include <fmt/format.h>
#    define TKIT_FORMAT(...) fmt::format(__VA_ARGS__)
#else
#    include <format>
#    define TKIT_FORMAT(...) std::format(__VA_ARGS__)
#endif

#define TKIT_LOG_COLOR_RESET "\033[0m"
#define TKIT_LOG_COLOR_RED "\033[31m"
#define TKIT_LOG_COLOR_GREEN "\033[32m"
#define TKIT_LOG_COLOR_YELLOW "\033[33m"
#define TKIT_LOG_COLOR_BLUE "\033[34m"

#ifdef TKIT_COMPILER_CLANG
#    define TKIT_DEBUG_BREAK() __builtin_debugtrap();
#elif defined(TKIT_COMPILER_GCC)
#    define TKIT_DEBUG_BREAK() __builtin_trap();
#elif defined(TKIT_COMPILER_MSVC)
#    define TKIT_DEBUG_BREAK() __debugbreak();
#elif defined(SIGTRAP)
#    define TKIT_DEBUG_BREAK() raise(SIGTRAP);
#elif defined(SIGABRT)
#    define TKIT_DEBUG_BREAK() raise(SIGABRT);
#endif

#define TKIT_DEBUG_BREAK_IF(p_Condition)                                                                               \
    if (p_Condition)                                                                                                   \
    TKIT_DEBUG_BREAK()

namespace TKit::Detail
{
#ifndef __TKIT_NO_LOGS
// These are not meant to be used directly, use the macros below instead
TKIT_API void LogMessage(const char *p_Level, std::string_view p_File, const i32 p_Line, const char *p_Color,
                         const bool p_Crash, std::string_view p_Message) noexcept;
#endif
} // namespace TKit::Detail

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_LOG_INFO(...)                                                                                         \
        TKit::Detail::LogMessage("INFO", __FILE__, Limits<i32>::max(), TKIT_LOG_COLOR_GREEN, false,                    \
                                 TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_LOG_INFO_IF(p_Condition, ...)                                                                         \
        if (p_Condition)                                                                                               \
        TKit::Detail::LogMessage("INFO", __FILE__, Limits<i32>::max(), TKIT_LOG_COLOR_GREEN, false,                    \
                                 TKIT_FORMAT(__VA_ARGS__))
#else
#    define TKIT_LOG_INFO(...)
#    define TKIT_LOG_INFO_IF(...)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        TKit::Detail::LogMessage("WARNING", __FILE__, __LINE__, TKIT_LOG_COLOR_YELLOW, false, TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_LOG_WARNING_IF(p_Condition, ...)                                                                      \
        if (p_Condition)                                                                                               \
        TKit::Detail::LogMessage("WARNING", __FILE__, __LINE__, TKIT_LOG_COLOR_YELLOW, false, TKIT_FORMAT(__VA_ARGS__))
#else
#    define TKIT_LOG_WARNING(...)
#    define TKIT_LOG_WARNING_IF(...)
#endif

#ifdef TKIT_ENABLE_ASSERTS

#    define TKIT_ERROR(...)                                                                                            \
        TKit::Detail::LogMessage("ERROR", __FILE__, __LINE__, TKIT_LOG_COLOR_RED, true, TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_ASSERT(p_Condition, ...)                                                                              \
        if (!(p_Condition))                                                                                            \
        TKit::Detail::LogMessage("ERROR", __FILE__, __LINE__, TKIT_LOG_COLOR_RED, true, TKIT_FORMAT(__VA_ARGS__))

#    define TKIT_ASSERT_RETURNS(expression, expected, ...) TKIT_ASSERT((expression) == (expected), __VA_ARGS__)
#else
#    define TKIT_ERROR(...)
#    define TKIT_ASSERT(...)
#    define TKIT_ASSERT_RETURNS(expression, expected, ...) expression
#endif
