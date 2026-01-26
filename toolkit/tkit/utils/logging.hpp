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
#ifdef TKIT_ENABLE_DEBUG_LOGS
inline thread_local bool t_DisabledDebugLogs = false;
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
inline thread_local bool t_DisabledInfoLogs = false;
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
inline thread_local bool t_DisabledWarningLogs = false;
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
inline thread_local bool t_DisabledErrorLogs = false;
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
        if (!TKit::Detail::t_DisabledDebugLogs)                                                                        \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF(condition, ...)                                                                          \
        if (!TKit::Detail::t_DisabledDebugLogs && (condition))                                                         \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_DEBUG_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_DEBUG_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_DEBUG_LOGS(disable) TKit::Detail::t_DisabledDebugLogs = disable

#else
#    define TKIT_LOG_DEBUG(message, ...)
#    define TKIT_LOG_DEBUG_IF(condition, message, ...)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_DEBUG_LOGS(disable)
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_LOG_INFO(...)                                                                                         \
        if (!TKit::Detail::t_DisabledInfoLogs)                                                                         \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF(condition, ...)                                                                           \
        if (!TKit::Detail::t_DisabledInfoLogs && (condition))                                                          \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...)                                                        \
        TKIT_LOG_INFO_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...)                                                    \
        TKIT_LOG_INFO_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_INFO_LOGS(disable) TKit::Detail::t_DisabledInfoLogs = disable

#else
#    define TKIT_LOG_INFO(message, ...)
#    define TKIT_LOG_INFO_IF(condition, message, ...)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_INFO_LOGS(disable)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        if (!TKit::Detail::t_DisabledWarningLogs)                                                                      \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF(condition, ...)                                                                        \
        if (!TKit::Detail::t_DisabledWarningLogs && (condition))                                                       \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...)                                                     \
        TKIT_LOG_WARNING_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...)                                                 \
        TKIT_LOG_WARNING_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_WARNING_LOGS(disable) TKit::Detail::t_DisabledWarningLogs = disable

#else
#    define TKIT_LOG_WARNING(message, ...)
#    define TKIT_LOG_WARNING_IF(condition, message, ...)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_WARNING_LOGS(disable)
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define TKIT_LOG_ERROR(...)                                                                                        \
        if (!TKit::Detail::t_DisabledErrorLogs)                                                                        \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF(condition, ...)                                                                          \
        if (!TKit::Detail::t_DisabledErrorLogs && (condition))                                                         \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_ERROR_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_ERROR_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_ERROR_LOGS(disable) TKit::Detail::t_DisabledErrorLogs = disable

#else
#    define TKIT_LOG_ERROR(message, ...)
#    define TKIT_LOG_ERROR_IF(condition, message, ...)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_ERROR_LOGS(disable)
#endif
#undef CREATE_LOGGING_FUNCTIONS
} // namespace TKit
