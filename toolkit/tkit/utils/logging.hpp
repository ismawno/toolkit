#pragma once

#include "tkit/utils/alias.hpp"
#include <string_view>
#include <fmt/format.h>

#define TKIT_LOG_COLOR_RESET "\033[0m"
#define TKIT_LOG_COLOR_DEBUG "\033[34m"
#define TKIT_LOG_COLOR_INFO "\033[32m"
#define TKIT_LOG_COLOR_WARNING "\033[33m"
#define TKIT_LOG_COLOR_ERROR "\033[31m"

#define CREATE_LOGGING_FUNCTIONS(type)                                                                                 \
    template <typename... Args> void Print(const type string, Args &&...args)                                          \
    {                                                                                                                  \
        fmt::print(string, std::forward<Args>(args)...);                                                               \
    }                                                                                                                  \
    template <typename... Args> void PrintLine(const type string, Args &&...args)                                      \
    {                                                                                                                  \
        fmt::println(string, std::forward<Args>(args)...);                                                             \
    }                                                                                                                  \
    template <typename... Args> constexpr auto Format(const type string, Args &&...args)                               \
    {                                                                                                                  \
        return fmt::format(string, std::forward<Args>(args)...);                                                       \
    }

#define CREATE_DETAIL_LOGGING_FUNCTIONS(type)                                                                          \
    template <typename... Args> void Log(const type string, const char *level, const char *color, Args &&...args)      \
    {                                                                                                                  \
        Log(fmt::format(string, std::forward<Args>(args)...), level, color);                                           \
    }                                                                                                                  \
    template <typename... Args>                                                                                        \
    void Log(const type string, const char *level, const char *color, const char *file, const i32 line,                \
             Args &&...args)                                                                                           \
    {                                                                                                                  \
        Log(fmt::format(string, std::forward<Args>(args)...), level, color, file, line);                               \
    }

namespace TKit::Detail
{
#define TKIT_DEBUG_LOGS_BIT (1 << 0)
#define TKIT_INFO_LOGS_BIT (1 << 1)
#define TKIT_WARNING_LOGS_BIT (1 << 2)
#define TKIT_ERROR_LOGS_BIT (1 << 3)

#if defined(TKIT_ENABLE_DEBUG_LOGS) || defined(TKIT_ENABLE_INFO_LOGS) || defined(TKIT_ENABLE_WARNING_LOGS) ||          \
    defined(TKIT_ENABLE_ERROR_LOGS)

#    ifndef TKIT_LOGS_MAX_STACK
#        define TKIT_LOGS_MAX_STACK 8
#    endif

inline thread_local u8 t_LogMask =
    TKIT_DEBUG_LOGS_BIT | TKIT_INFO_LOGS_BIT | TKIT_WARNING_LOGS_BIT | TKIT_ERROR_LOGS_BIT;
inline thread_local u8 t_LogStack[TKIT_LOGS_MAX_STACK];
inline thread_local usize t_LogIndex = 0;

#    define TKIT_LOGS_PUSH() TKit::Detail::t_LogStack[TKit::Detail::t_LogIndex++] = TKit::Detail::t_LogMask
#    define TKIT_LOGS_POP() TKit::Detail::t_LogMask = TKit::Detail::t_LogStack[--TKit::Detail::t_LogIndex]

#    define TKIT_LOGS_ENABLE(mask) TKit::Detail::t_LogMask |= (mask)
#    define TKIT_LOGS_DISABLE(mask) TKit::Detail::t_LogMask &= ~(mask)

#else

#    define TKIT_LOGS_PUSH()
#    define TKIT_LOGS_POP()

#    define TKIT_LOGS_ENABLE(mask)
#    define TKIT_LOGS_DISABLE(mask)

#endif

void Log(std::string_view message, const char *level, const char *color);
void Log(std::string_view message, const char *level, const char *color, const char *file, i32 line);

CREATE_DETAIL_LOGGING_FUNCTIONS(fmt::format_string<Args...>)
CREATE_DETAIL_LOGGING_FUNCTIONS(fmt::runtime_format_string<>)

#undef CREATE_DETAIL_LOGGING_FUNCTIONS
} // namespace TKit::Detail

namespace TKit
{
fmt::runtime_format_string<> RuntimeString(std::string_view string);

CREATE_LOGGING_FUNCTIONS(fmt::format_string<Args...>)
CREATE_LOGGING_FUNCTIONS(fmt::runtime_format_string<>)

#ifdef TKIT_ENABLE_DEBUG_LOGS
#    define TKIT_LOG_DEBUG(...)                                                                                        \
        if (TKit::Detail::t_LogMask & TKIT_DEBUG_LOGS_BIT)                                                             \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF(condition, ...)                                                                          \
        if ((TKit::Detail::t_LogMask & TKIT_DEBUG_LOGS_BIT) && (condition))                                            \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_DEBUG_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_DEBUG_IF((expression) != (expected), __VA_ARGS__)

#else
#    define TKIT_LOG_DEBUG(message, ...)
#    define TKIT_LOG_DEBUG_IF(condition, message, ...)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...) expression
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_LOG_INFO(...)                                                                                         \
        if (TKit::Detail::t_LogMask & TKIT_INFO_LOGS_BIT)                                                              \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF(condition, ...)                                                                           \
        if ((TKit::Detail::t_LogMask & TKIT_INFO_LOGS_BIT) && (condition))                                             \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...)                                                        \
        TKIT_LOG_INFO_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...)                                                    \
        TKIT_LOG_INFO_IF((expression) != (expected), __VA_ARGS__)

#else
#    define TKIT_LOG_INFO(message, ...)
#    define TKIT_LOG_INFO_IF(condition, message, ...)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...) expression
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        if (TKit::Detail::t_LogMask & TKIT_WARNING_LOGS_BIT)                                                           \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF(condition, ...)                                                                        \
        if ((TKit::Detail::t_LogMask & TKIT_WARNING_LOGS_BIT) && (condition))                                          \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...)                                                     \
        TKIT_LOG_WARNING_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...)                                                 \
        TKIT_LOG_WARNING_IF((expression) != (expected), __VA_ARGS__)

#else
#    define TKIT_LOG_WARNING(message, ...)
#    define TKIT_LOG_WARNING_IF(condition, message, ...)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...) expression
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define TKIT_LOG_ERROR(...)                                                                                        \
        if (TKit::Detail::t_LogMask & TKIT_ERROR_LOGS_BIT)                                                             \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF(condition, ...)                                                                          \
        if ((TKit::Detail::t_LogMask & TKIT_ERROR_LOGS_BIT) && (condition))                                            \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_ERROR_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_ERROR_IF((expression) != (expected), __VA_ARGS__)

#else
#    define TKIT_LOG_ERROR(message, ...)
#    define TKIT_LOG_ERROR_IF(condition, message, ...)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...) expression
#endif
#undef CREATE_LOGGING_FUNCTIONS
} // namespace TKit
