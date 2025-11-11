#pragma once

#ifndef TKIT_ENABLE_LOGGING
#    error                                                                                                             \
        "[TOOLKIT][LOGGING] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_LOGGING"
#endif

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/alias.hpp"
#include <string_view>
#include <fmt/format.h>

#define TKIT_LOG_COLOR_RESET "\033[0m"
#define TKIT_LOG_COLOR_INFO "\033[32m"
#define TKIT_LOG_COLOR_WARNING "\033[33m"
#define TKIT_LOG_COLOR_ERROR "\033[31m"

namespace TKit
{
template <typename... Args> constexpr auto Format(const fmt::format_string<Args...> p_String, Args &&...p_Args)
{
    return fmt::format(p_String, std::forward<Args>(p_Args)...);
}
template <typename... Args> constexpr auto Format(const fmt::runtime_format_string<Args...> p_String, Args &&...p_Args)
{
    return fmt::format(p_String, std::forward<Args>(p_Args)...);
}

template <typename... Args> void Print(const fmt::format_string<Args...> p_String, Args &&...p_Args)
{
    fmt::print(p_String, std::forward<Args>(p_Args)...);
}
template <typename... Args> void PrintLine(const fmt::format_string<Args...> p_String, Args &&...p_Args)
{
    fmt::println(p_String, std::forward<Args>(p_Args)...);
}

TKIT_API void Log(std::string_view p_Message, const char *p_Level, const char *p_Color);
TKIT_API void Log(std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File, i32 p_Line);

template <typename... Args>
void Log(const fmt::format_string<Args...> p_String, const char *p_Level, const char *p_Color, Args &&...p_Args)
{
    Log(fmt::format(p_String, std::forward<Args>(p_Args)...), p_Level, p_Color);
}
template <typename... Args>
void Log(const fmt::format_string<Args...> p_String, const char *p_Level, const char *p_Color, const char *p_File,
         const i32 p_Line, Args &&...p_Args)
{
    Log(fmt::format(p_String, std::forward<Args>(p_Args)...), p_Level, p_Color, p_File, p_Line);
}

template <typename... Args>
TKIT_API void Log(std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File, i32 p_Line);

template <typename... Args> void Info(const fmt::format_string<Args...> p_String, Args &&...p_Args)
{
    Log(p_String, "INFO", TKIT_LOG_COLOR_INFO, std::forward<Args>(p_Args)...);
}
template <typename... Args> void Warning(const fmt::format_string<Args...> p_String, Args &&...p_Args)
{
    Log(p_String, "WARNING", TKIT_LOG_COLOR_INFO, std::forward<Args>(p_Args)...);
}
template <typename... Args> void Error(const fmt::format_string<Args...> p_String, Args &&...p_Args)
{
    Log(p_String, "ERROR", TKIT_LOG_COLOR_INFO, std::forward<Args>(p_Args)...);
}

} // namespace TKit
