#pragma once

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/alias.hpp"
#include <string_view>
#include <fmt/format.h>

#define TKIT_LOG_COLOR_RESET "\033[0m"
#define TKIT_LOG_COLOR_DEBUG "\033[34m"
#define TKIT_LOG_COLOR_INFO "\033[32m"
#define TKIT_LOG_COLOR_WARNING "\033[33m"
#define TKIT_LOG_COLOR_ERROR "\033[31m"

#define CREATE_LOGGING_FUNCTIONS(p_Type)                                                                               \
    template <typename... Args> void Print(const p_Type p_String, Args &&...p_Args)                                    \
    {                                                                                                                  \
        fmt::print(p_String, std::forward<Args>(p_Args)...);                                                           \
    }                                                                                                                  \
    template <typename... Args> void PrintLine(const p_Type p_String, Args &&...p_Args)                                \
    {                                                                                                                  \
        fmt::println(p_String, std::forward<Args>(p_Args)...);                                                         \
    }                                                                                                                  \
    template <typename... Args> constexpr auto Format(const p_Type p_String, Args &&...p_Args)                         \
    {                                                                                                                  \
        return fmt::format(p_String, std::forward<Args>(p_Args)...);                                                   \
    }

#define CREATE_DETAIL_LOGGING_FUNCTIONS(p_Type)                                                                        \
    template <typename... Args>                                                                                        \
    void Log(const p_Type p_String, const char *p_Level, const char *p_Color, Args &&...p_Args)                        \
    {                                                                                                                  \
        Log(fmt::format(p_String, std::forward<Args>(p_Args)...), p_Level, p_Color);                                   \
    }                                                                                                                  \
    template <typename... Args>                                                                                        \
    void Log(const p_Type p_String, const char *p_Level, const char *p_Color, const char *p_File, const i32 p_Line,    \
             Args &&...p_Args)                                                                                         \
    {                                                                                                                  \
        Log(fmt::format(p_String, std::forward<Args>(p_Args)...), p_Level, p_Color, p_File, p_Line);                   \
    }

namespace TKit::Detail
{
#ifdef TKIT_ENABLE_DEBUG_LOGS
inline static thread_local bool t_DisabledDebugLogs = false;
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
inline static thread_local bool t_DisabledInfoLogs = false;
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
inline static thread_local bool t_DisabledWarningLogs = false;
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
inline static thread_local bool t_DisabledErrorLogs = false;
#endif

void Log(std::string_view p_Message, const char *p_Level, const char *p_Color);
void Log(std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File, i32 p_Line);

CREATE_DETAIL_LOGGING_FUNCTIONS(fmt::format_string<Args...>)
CREATE_DETAIL_LOGGING_FUNCTIONS(fmt::runtime_format_string<>)

#undef CREATE_DETAIL_LOGGING_FUNCTIONS
} // namespace TKit::Detail

namespace TKit
{
fmt::runtime_format_string<> RuntimeString(std::string_view p_String);

CREATE_LOGGING_FUNCTIONS(fmt::format_string<Args...>)
CREATE_LOGGING_FUNCTIONS(fmt::runtime_format_string<>)

#ifdef TKIT_ENABLE_DEBUG_LOGS
#    define TKIT_LOG_DEBUG(...)                                                                                        \
        if (!TKit::Detail::t_DisabledDebugLogs)                                                                        \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF(p_Condition, ...)                                                                        \
        if (!TKit::Detail::t_DisabledDebugLogs && (p_Condition))                                                       \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_DEBUG_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_DEBUG_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_DEBUG_LOGS(p_Disable) TKit::Detail::t_DisabledDebugLogs = p_Disable

#else
#    define TKIT_LOG_DEBUG(p_Message, ...)
#    define TKIT_LOG_DEBUG_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_DEBUG_LOGS(p_Disable)
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_LOG_INFO(...)                                                                                         \
        if (!TKit::Detail::t_DisabledInfoLogs)                                                                         \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF(p_Condition, ...)                                                                         \
        if (!TKit::Detail::t_DisabledInfoLogs && (p_Condition))                                                        \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...)                                                        \
        TKIT_LOG_INFO_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...)                                                    \
        TKIT_LOG_INFO_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_INFO_LOGS(p_Disable) TKit::Detail::t_DisabledInfoLogs = p_Disable

#else
#    define TKIT_LOG_INFO(p_Message, ...)
#    define TKIT_LOG_INFO_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_INFO_LOGS(p_Disable)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        if (!TKit::Detail::t_DisabledWarningLogs)                                                                      \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF(p_Condition, ...)                                                                      \
        if (!TKit::Detail::t_DisabledWarningLogs && (p_Condition))                                                     \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...)                                                     \
        TKIT_LOG_WARNING_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...)                                                 \
        TKIT_LOG_WARNING_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_WARNING_LOGS(p_Disable) TKit::Detail::t_DisabledWarningLogs = p_Disable

#else
#    define TKIT_LOG_WARNING(p_Message, ...)
#    define TKIT_LOG_WARNING_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_WARNING_LOGS(p_Disable)
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define TKIT_LOG_ERROR(...)                                                                                        \
        if (!TKit::Detail::t_DisabledErrorLogs)                                                                        \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF(p_Condition, ...)                                                                        \
        if (!TKit::Detail::t_DisabledErrorLogs && (p_Condition))                                                       \
        TKit::Detail::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_ERROR_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_ERROR_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_ERROR_LOGS(p_Disable) TKit::Detail::t_DisabledErrorLogs = p_Disable

#else
#    define TKIT_LOG_ERROR(p_Message, ...)
#    define TKIT_LOG_ERROR_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_ERROR_LOGS(p_Disable)
#endif
#undef CREATE_LOGGING_FUNCTIONS
} // namespace TKit
