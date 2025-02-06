#pragma once

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/alias.hpp"

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

#ifndef __TKIT_NO_LOGS
#    include <atomic>
#    include <string_view>
#endif

namespace TKit::Detail
{
#ifndef __TKIT_NO_LOGS
// These are not meant to be used directly, use the macros below instead
TKIT_API void LogMessage(const char *p_Level, std::string_view p_File, const i32 p_Line, const char *p_Color,
                         const bool p_Crash, std::string_view p_Message) noexcept;

#    ifdef TKIT_OS_LINUX
template <typename... Args> auto Format(const fmt::format_string<Args...> p_Format, Args &&...p_Args) noexcept
{
    return TKIT_FORMAT(p_Format, std::forward<Args>(p_Args)...);
}
#    else
template <typename... Args> auto Format(const std::format_string<Args...> p_Format, Args &&...p_Args) noexcept
{
    return TKIT_FORMAT(p_Format, std::forward<Args>(p_Args)...);
}
#    endif

constexpr const char *Format() noexcept
{
    return "";
}
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
inline std::atomic_flag DisabledInfoLogs = ATOMIC_FLAG_INIT;
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
inline std::atomic_flag DisabledWarningLogs = ATOMIC_FLAG_INIT;
#endif

#ifdef TKIT_ENABLE_ASSERTS
inline std::atomic_flag DisabledAsserts = ATOMIC_FLAG_INIT;
#endif
} // namespace TKit::Detail

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_LOG_INFO(...)                                                                                         \
        if (!TKit::Detail::DisabledInfoLogs.test(std::memory_order_relaxed))                                           \
        TKit::Detail::LogMessage("INFO", __FILE__, Limits<i32>::max(), TKIT_LOG_COLOR_GREEN, false,                    \
                                 TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_LOG_INFO_IF(p_Condition, ...)                                                                         \
        if ((p_Condition) && !TKit::Detail::DisabledInfoLogs.test(std::memory_order_relaxed))                          \
        TKit::Detail::LogMessage("INFO", __FILE__, Limits<i32>::max(), TKIT_LOG_COLOR_GREEN, false,                    \
                                 TKIT_FORMAT(__VA_ARGS__))

#    define TKIT_IGNORE_INFO_LOGS_PUSH() (void)TKit::Detail::DisabledInfoLogs.test_and_set()
#    define TKIT_IGNORE_INFO_LOGS_POP() TKit::Detail::DisabledInfoLogs.clear()
#else
#    define TKIT_LOG_INFO(...)
#    define TKIT_LOG_INFO_IF(...)

#    define TKIT_IGNORE_INFO_LOGS_PUSH()
#    define TKIT_IGNORE_INFO_LOGS_POP()
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        if (!TKit::Detail::DisabledWarningLogs.test(std::memory_order_relaxed))                                        \
        TKit::Detail::LogMessage("WARNING", __FILE__, __LINE__, TKIT_LOG_COLOR_YELLOW, false, TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_LOG_WARNING_IF(p_Condition, ...)                                                                      \
        if ((p_Condition) && !TKit::Detail::DisabledWarningLogs.test(std::memory_order_relaxed))                       \
        TKit::Detail::LogMessage("WARNING", __FILE__, __LINE__, TKIT_LOG_COLOR_YELLOW, false, TKIT_FORMAT(__VA_ARGS__))

#    define TKIT_IGNORE_WARNING_LOGS_PUSH() (void)TKit::Detail::DisabledWarningLogs.test_and_set()
#    define TKIT_IGNORE_WARNING_LOGS_POP() TKit::Detail::DisabledWarningLogs.clear()
#else
#    define TKIT_LOG_WARNING(...)
#    define TKIT_LOG_WARNING_IF(...)

#    define TKIT_IGNORE_WARNING_LOGS_PUSH()
#    define TKIT_IGNORE_WARNING_LOGS_POP()
#endif

#ifdef TKIT_ENABLE_ASSERTS

#    define TKIT_ERROR(...)                                                                                            \
        if (!TKit::Detail::DisabledAsserts.test(std::memory_order_relaxed))                                            \
        TKit::Detail::LogMessage("ERROR", __FILE__, __LINE__, TKIT_LOG_COLOR_RED, true,                                \
                                 TKit::Detail::Format(__VA_ARGS__))
#    define TKIT_ASSERT(p_Condition, ...)                                                                              \
        if (!(p_Condition) && !TKit::Detail::DisabledAsserts.test(std::memory_order_relaxed))                          \
        TKit::Detail::LogMessage("ERROR", __FILE__, __LINE__, TKIT_LOG_COLOR_RED, true,                                \
                                 TKit::Detail::Format(__VA_ARGS__))

#    define TKIT_ASSERT_RETURNS(expression, expected, ...) TKIT_ASSERT((expression) == (expected), __VA_ARGS__)

#    define TKIT_IGNORE_ASSERTS_PUSH() (void)TKit::Detail::DisabledAsserts.test_and_set()
#    define TKIT_IGNORE_ASSERTS_POP() TKit::Detail::DisabledAsserts.clear()
#else
#    define TKIT_ERROR(...)
#    define TKIT_ASSERT(...)
#    define TKIT_ASSERT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_ASSERTS_PUSH()
#    define TKIT_IGNORE_ASSERTS_POP()
#endif
